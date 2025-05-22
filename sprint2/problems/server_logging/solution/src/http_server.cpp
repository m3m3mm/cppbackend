// src/http_server.cpp
#include "http_server.h"
#include "logging.h"
#include <iostream>
#include <boost/json.hpp> // Для сериализации тел ошибок в JSON в Session::OnRead

namespace http_server {

Listener::Listener(net::io_context& ioc, tcp::endpoint endpoint, RequestHandler&& handler)
    : ioc_(ioc)
    , acceptor_(ioc)
    , handler_(std::move(handler)) { // Listener хранит свой экземпляр RequestHandler
    beast::error_code ec;

    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        logging::LogError(ec.value(), ec.message(), "accept");
        throw std::runtime_error("Failed to open acceptor: " + ec.message());
    }

    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
        logging::LogError(ec.value(), ec.message(), "accept");
        throw std::runtime_error("Failed to set reuse address: " + ec.message());
    }

    acceptor_.bind(endpoint, ec);
    if (ec) {
        logging::LogError(ec.value(), ec.message(), "accept");
        throw std::runtime_error("Failed to bind: " + ec.message());
    }

    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
        logging::LogError(ec.value(), ec.message(), "accept");
        throw std::runtime_error("Failed to start listening: " + ec.message());
    }
}

void Listener::Run() {
    DoAccept();
}

void Listener::DoAccept() {
    acceptor_.async_accept(
        [self = shared_from_this()](beast::error_code ec, tcp::socket socket) {
            self->OnAccept(ec, std::move(socket));
        });
}

void Listener::OnAccept(beast::error_code ec, tcp::socket socket) {
    if (ec) {
        logging::LogError(ec.value(), ec.message(), "accept");
    } else {
        // Создаём сессию и запускаем её.
        // handler_ (lvalue, член Listener) будет скопирован в параметр конструктора Session,
        // т.к. конструктор Session теперь принимает RequestHandler по значению.
        // Затем этот параметр будет перемещён в член handler_ сессии.
        std::make_shared<Session>(std::move(socket), handler_)->Run();
    }

    // Принимаем следующее соединение
    DoAccept();
}

// Конструктор Session принимает RequestHandler по значению
Session::Session(tcp::socket&& socket, RequestHandler handler) // handler здесь - копия или перемещенный объект
    : socket_(std::move(socket))
    , handler_(std::move(handler)) { // Перемещаем из параметра в член класса
}

void Session::Run() {
    DoRead();
}

void Session::DoRead() {
    req_ = {}; // Очищаем запрос перед чтением
    http::async_read(socket_, buffer_, req_,
        [self = shared_from_this()](beast::error_code ec, std::size_t bytes_transferred) {
            self->OnRead(ec, bytes_transferred);
        });
}

void Session::OnRead(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec == http::error::end_of_stream)
        return DoClose();

    if (ec) {
        logging::LogError(ec.value(), ec.message(), "read");
        return DoClose(); // Закрываем соединение при ошибке чтения
    }

    // Лямбда для отправки ответа, захватывает self
    auto send_cb = [self = shared_from_this()](http::response<http::string_body>&& res) {
        self->res_ = std::move(res);
        http::async_write(
            self->socket_,
            self->res_,
            // ИЗМЕНЕНО: Лямбда для async_write теперь корректно принимает bytes_transferred
            [self](beast::error_code write_ec, std::size_t written_bytes) { // Используем другое имя для параметра
                // ИЗМЕНЕНО: Передаём written_bytes (переименованный bytes_transferred) в OnWrite
                self->OnWrite(write_ec, written_bytes, self->res_.need_eof());
            });
    };
    
    try {
        // Передаём управление обработчику запросов
        handler_(std::move(req_), std::move(send_cb));
    } catch (const std::exception& e) {
        logging::LogError(500, e.what(), "request");
        // Формируем и отправляем ответ 500 Internal Server Error
        http::response<http::string_body> res{http::status::internal_server_error, req_.version()};
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req_.keep_alive()); // Используем req_ из сессии, он еще действителен
        boost::json::object error_body;
        error_body["code"] = "internalError";
        error_body["message"] = "An internal server error occurred.";
        res.body() = boost::json::serialize(error_body);
        res.prepare_payload();
        send_cb(std::move(res)); // Отправляем через наш колбэк
    } catch (...) {
        logging::LogError(500, "Unknown exception", "request");
        http::response<http::string_body> res{http::status::internal_server_error, req_.version()};
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req_.keep_alive());
        boost::json::object error_body;
        error_body["code"] = "internalError";
        error_body["message"] = "An unknown internal server error occurred.";
        res.body() = boost::json::serialize(error_body);
        res.prepare_payload();
        send_cb(std::move(res));
    }
}

void Session::OnWrite(beast::error_code ec, std::size_t bytes_transferred, bool close) {
    boost::ignore_unused(bytes_transferred); // bytes_transferred теперь снова передается

    if (ec) {
        logging::LogError(ec.value(), ec.message(), "write");
        return;
    }

    if (close) {
        return DoClose();
    }

    DoRead(); // Читаем следующий запрос
}

void Session::DoClose() {
    beast::error_code ec;
    socket_.shutdown(tcp::socket::shutdown_both, ec);
    if (ec) {
        logging::LogError(ec.value(), ec.message(), "close");
    }
}

void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler) {
    std::make_shared<Listener>(ioc, endpoint, std::move(handler))->Run();
}

}  // namespace http_server

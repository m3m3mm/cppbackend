#pragma once
#include "sdk.h"
// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace http_server {

namespace net = boost::asio;
using tcp = net::ip::tcp;
namespace beast = boost::beast;
namespace http = beast::http;

class SessionBase {
public:
    // Запрещаем копирование и присваивание объектов SessionBase
    SessionBase(const SessionBase&) = delete;
    SessionBase& operator=(const SessionBase&) = delete;

    // Разрешаем перемещение объектов SessionBase
    SessionBase(SessionBase&&) = default;
    SessionBase& operator=(SessionBase&&) = default;

    virtual ~SessionBase() = default;

protected:
    // Разрешаем конструирование и уничтожение только в производных классах
    SessionBase() = default;
};

template <typename RequestHandler>
class Session : public SessionBase, public std::enable_shared_from_this<Session<RequestHandler>> {
public:
    template <typename Handler>
    Session(tcp::socket&& socket, Handler&& request_handler)
        : socket_(std::move(socket))
        , request_handler_(std::forward<Handler>(request_handler)) {
    }

    void Run() {
        // Запускаем асинхронное чтение запроса
        DoRead();
    }

private:
    // Чтение запроса
    void DoRead() {
        // Очищаем запрос от прежнего значения
        request_ = {};

        // Устанавливаем предельный размер тела запроса = 10 МБ
        beast::http::parser<true, beast::http::string_body> parser;
        parser.body_limit(1024 * 1024 * 10);

        // Асинхронно читаем запрос
        http::async_read(socket_, buffer_, request_,
            beast::bind_front_handler(&Session::OnRead, this->shared_from_this()));
    }

    // Обработка запроса
    void OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
        if (ec == http::error::end_of_stream) {
            // Нормальная ситуация - клиент закрыл соединение
            return DoClose();
        }
        if (ec) {
            // Обрабатываем ошибку чтения запроса
            return;
        }

        // Обрабатываем запрос, используя функцию-обработчик запроса
        HandleRequest(std::move(request_));
    }

    void HandleRequest(http::request<http::string_body>&& request) {
        // Создаем лямбда-функцию для отправки ответа
        auto send_response = [self = this->shared_from_this()](auto&& response) {
            // Захватываем объект session в лямбда-функцию, чтобы продлить время его жизни
            self->response_ = std::forward<decltype(response)>(response);

            // Отправляем ответ клиенту
            http::async_write(
                self->socket_, self->response_,
                beast::bind_front_handler(&Session::OnWrite, self->shared_from_this(), self->response_.need_eof()));
        };

        // Используем обработчик запроса, передавая ему запрос и лямбда-функцию для отправки ответа
        request_handler_(std::move(request), send_response);
    }

    // Отправка ответа клиенту
    void OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written) {
        if (ec) {
            // Обрабатываем ошибку записи ответа
            return;
        }

        if (close) {
            // Закрываем соединение
            return DoClose();
        }

        // Считываем следующий запрос
        DoRead();
    }

    // Закрытие соединения
    void DoClose() {
        beast::error_code ec;
        socket_.shutdown(tcp::socket::shutdown_send, ec);
    }

private:
    tcp::socket socket_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> request_;
    http::response<http::string_body> response_;
    RequestHandler request_handler_;
};

template <typename RequestHandler>
class Listener : public std::enable_shared_from_this<Listener<RequestHandler>> {
public:
    template <typename Handler>
    Listener(net::io_context& ioc, const tcp::endpoint& endpoint, Handler&& request_handler)
        : acceptor_(net::make_strand(ioc))
        , request_handler_(std::forward<Handler>(request_handler)) {
        
        // Открываем acceptor
        beast::error_code ec;
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            return;
        }

        // Разрешаем переиспользовать адрес
        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        if (ec) {
            return;
        }

        // Биндим endpoint
        acceptor_.bind(endpoint, ec);
        if (ec) {
            return;
        }

        // Начинаем слушать входящие соединения
        acceptor_.listen(net::socket_base::max_listen_connections, ec);
        if (ec) {
            return;
        }
    }

    void Run() {
        // Асинхронно принимаем входящие соединения
        DoAccept();
    }

private:
    void DoAccept() {
        // Асинхронно ожидаем нового соединения
        acceptor_.async_accept(
            net::make_strand(acceptor_.get_executor()),
            beast::bind_front_handler(&Listener::OnAccept, this->shared_from_this()));
    }

    void OnAccept(beast::error_code ec, tcp::socket socket) {
        if (ec) {
            // Обрабатываем ошибку приема соединения
        } else {
            // Создаём и запускаем новую сессию
            std::make_shared<Session<RequestHandler>>(std::move(socket), request_handler_)->Run();
        }

        // Принимаем новое соединение
        DoAccept();
    }

private:
    tcp::acceptor acceptor_;
    RequestHandler request_handler_;
};

template <typename RequestHandler>
void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler) {
    // Создаём и запускаем обработчик соединений
    auto listener = std::make_shared<Listener<RequestHandler>>(
        ioc, endpoint, std::forward<RequestHandler>(handler));
    listener->Run();
}

}  // namespace http_server

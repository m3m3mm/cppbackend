// src/http_server.h
#pragma once
#include "sdk.h"
//
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <string>
#include <memory>
#include <functional> // Убедимся, что std::function доступен

namespace http_server {

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

// Функциональный объект для обработки HTTP-запросов
using RequestHandler = std::function<void(http::request<http::string_body>&& req, 
                                        std::function<void(http::response<http::string_body>&&)>&& send)>;

// Принимает входящие соединения и запускает сессии
class Listener : public std::enable_shared_from_this<Listener> {
public:
    Listener(net::io_context& ioc, tcp::endpoint endpoint, RequestHandler&& handler);

    // Начать приём входящих соединений
    void Run();

private:
    void DoAccept();
    void OnAccept(beast::error_code ec, tcp::socket socket);

    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    RequestHandler handler_; // Хранит копию обработчика для всех сессий
};

// Обрабатывает HTTP-соединение с сервером
class Session : public std::enable_shared_from_this<Session> {
public:
    // Принимает владение сокетом
    // ИЗМЕНЕНО: Принимаем RequestHandler по значению для упрощения копирования/перемещения
    explicit Session(tcp::socket&& socket, RequestHandler handler); 

    // Запустить асинхронную операцию
    void Run();

private:
    void DoRead();
    void OnRead(beast::error_code ec, std::size_t bytes_transferred);
    void OnWrite(beast::error_code ec, std::size_t bytes_transferred, bool close);
    void DoClose();

    tcp::socket socket_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
    http::response<http::string_body> res_;
    RequestHandler handler_; // Каждая сессия имеет свою копию (или перемещенный экземпляр) обработчика
};

// Запустить HTTP-сервер
void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler);

}  // namespace http_server

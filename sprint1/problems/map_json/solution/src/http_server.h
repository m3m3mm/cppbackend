#pragma once
#include "sdk.h"
//
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <string>
#include <memory>

namespace http_server {

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

// The function object to handle HTTP requests
using RequestHandler = std::function<void(http::request<http::string_body>&& req, 
                                        std::function<void(http::response<http::string_body>&&)>&& send)>;

// Accepts incoming connections and launches the sessions
class Listener : public std::enable_shared_from_this<Listener> {
public:
    Listener(net::io_context& ioc, tcp::endpoint endpoint, RequestHandler&& handler);

    // Start accepting incoming connections
    void Run();

private:
    void DoAccept();
    void OnAccept(beast::error_code ec, tcp::socket socket);

    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    RequestHandler handler_;
};

// Handles an HTTP server connection
class Session : public std::enable_shared_from_this<Session> {
public:
    // Take ownership of the socket
    explicit Session(tcp::socket&& socket, RequestHandler&& handler);

    // Start the asynchronous operation
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
    RequestHandler handler_;
};

// Start the HTTP server
void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler);

}  // namespace http_server

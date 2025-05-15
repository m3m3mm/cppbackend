#include "http_server.h"
#include <iostream>
#include <boost/json.hpp> // For error response serialization in Session::OnRead

namespace http_server {

// Разместите здесь реализацию http-сервера, взяв её из задания по разработке асинхронного сервера

Listener::Listener(net::io_context& ioc, tcp::endpoint endpoint, RequestHandler&& handler)
    : ioc_(ioc)
    , acceptor_(ioc)
    , handler_(std::move(handler)) {
    beast::error_code ec;

    // Open the acceptor
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        throw std::runtime_error("Failed to open acceptor: " + ec.message());
    }

    // Allow address reuse
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
        throw std::runtime_error("Failed to set reuse address: " + ec.message());
    }

    // Bind to the server address
    acceptor_.bind(endpoint, ec);
    if (ec) {
        throw std::runtime_error("Failed to bind: " + ec.message());
    }

    // Start listening for connections
    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
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
        std::cerr << "Accept error: " << ec.message() << std::endl;
    } else {
        // Create the session and run it
        // handler_ (lvalue) will be copied into Session's constructor parameter
        // then moved into Session's handler_ member.
        std::make_shared<Session>(std::move(socket), handler_)->Run();
    }

    // Accept another connection
    DoAccept();
}

// Session constructor takes handler by value
Session::Session(tcp::socket&& socket, RequestHandler handler)
    : socket_(std::move(socket))
    , handler_(std::move(handler)) { // Move from the parameter
}

void Session::Run() {
    DoRead();
}

void Session::DoRead() {
    // Make the request empty before reading,
    // otherwise the operation behavior is undefined.
    req_ = {};

    // Read a request
    http::async_read(socket_, buffer_, req_,
        [self = shared_from_this()](beast::error_code ec, std::size_t bytes_transferred) {
            self->OnRead(ec, bytes_transferred);
        });
}

void Session::OnRead(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    // This means they closed the connection
    if (ec == http::error::end_of_stream)
        return DoClose();

    if (ec) {
        std::cerr << "Read error: " << ec.message() << std::endl;
        return DoClose();
    }

    // Send the response
    auto send_cb = [self = shared_from_this()](http::response<http::string_body>&& res) {
        self->res_ = std::move(res);
        http::async_write(
            self->socket_,
            self->res_,
            [self](beast::error_code write_ec, std::size_t write_bytes_transferred) { // FIXED: lambda signature
                self->OnWrite(write_ec, write_bytes_transferred, self->res_.need_eof()); // FIXED: pass bytes_transferred
            });
    };
    
    // Handle the request
    try {
        handler_(std::move(req_), std::move(send_cb));
    } catch (const std::exception& e) {
        std::cerr << "RequestHandler exception: " << e.what() << std::endl;
        http::response<http::string_body> res{http::status::internal_server_error, req_.version()};
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req_.keep_alive());
        boost::json::object error_body;
        error_body["code"] = "internalError";
        error_body["message"] = "An internal server error occurred.";
        res.body() = boost::json::serialize(error_body);
        res.prepare_payload();
        send_cb(std::move(res));
    } catch (...) {
        std::cerr << "Unknown exception in RequestHandler." << std::endl;
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
    boost::ignore_unused(bytes_transferred);

    if (ec) {
        std::cerr << "Write error: " << ec.message() << std::endl;
        return;
    }

    if (close) {
        // This means we should close the connection, usually because
        // the response indicated the "Connection: close" semantic.
        return DoClose();
    }

    // Read another request
    DoRead();
}

void Session::DoClose() {
    beast::error_code ec;
    socket_.shutdown(tcp::socket::shutdown_both, ec);
}

void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler) {
    // Create and launch a listening port
    std::make_shared<Listener>(ioc, endpoint, std::move(handler))->Run();
}

}  // namespace http_server

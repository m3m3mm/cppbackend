#include "http_server.h"
#include <iostream>

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
        std::make_shared<Session>(std::move(socket), std::move(handler_))->Run();
    }

    // Accept another connection
    DoAccept();
}

Session::Session(tcp::socket&& socket, RequestHandler&& handler)
    : socket_(std::move(socket))
    , handler_(std::move(handler)) {
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
        return;
    }

    // Send the response
    auto send = [self = shared_from_this()](http::response<http::string_body>&& res) {
        self->res_ = std::move(res);
        http::async_write(
            self->socket_,
            self->res_,
            [self](beast::error_code ec, std::size_t bytes_transferred) {
                self->OnWrite(ec, bytes_transferred, self->res_.need_eof());
            });
    };

    // Handle the request
    handler_(std::move(req_), std::move(send));
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

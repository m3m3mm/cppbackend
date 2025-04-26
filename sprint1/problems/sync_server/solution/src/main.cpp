#ifdef WIN32
#include <sdkddkver.h>
#endif

#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <thread>
#include <optional>

namespace net = boost::asio;
using tcp = net::ip::tcp;
using namespace std::literals;
namespace beast = boost::beast;
namespace http = beast::http;

void HandleRequest(http::request<http::string_body>&& req, http::response<http::string_body>& res) {
    if (req.method() == http::verb::get || req.method() == http::verb::head) {
	std::string target(req.target());
        if (!target.empty() && target[0] == '/') {
            target = target.substr(1); // удаляем ведущий '/'
        }
        std::string body = "Hello, " + target;
        
        res.result(http::status::ok);
        res.version(req.version());
        res.set(http::field::server, "SyncServer");
        res.set(http::field::content_type, "text/html");
        res.set(http::field::content_length, std::to_string(body.size()));
        
        if (req.method() == http::verb::get) {
            res.body() = body;
        } else if (req.method() == http::verb::head) {
            res.body() = ""; // тело пустое, но content-length как в GET
        }
    } else {
        std::string body = "Invalid method";
        res.result(http::status::method_not_allowed);
        res.version(req.version());
        res.set(http::field::server, "SyncServer");
        res.set(http::field::content_type, "text/html");
        res.set(http::field::allow, "GET, HEAD");
        res.set(http::field::content_length, std::to_string(body.size()));
        res.body() = body;
    }
}

int main() {
    try {
        net::io_context ioc;
        tcp::acceptor acceptor(ioc, {tcp::v4(), 8080});
        std::cout << "Server has started..."sv << std::endl;
        
        while (true) {
            tcp::socket socket(ioc);
            acceptor.accept(socket);
            
            beast::flat_buffer buffer;
            http::request<http::string_body> req;
            http::read(socket, buffer, req);
            
            http::response<http::string_body> res;
            HandleRequest(std::move(req), res);
            
            http::write(socket, res);
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
}

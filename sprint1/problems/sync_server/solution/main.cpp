#ifdef WIN32
#include <sdkddkver.h>
#endif
// boost.beast будет использовать std::string_view вместо boost::string_view
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

void handle_request(beast::tcp_stream& stream, beast::http::request<beast::http::string_body>& req) {
    beast::http::response<beast::http::string_body> res;

    // Обработка GET и HEAD запросов
    if (req.method() == http::verb::get || req.method() == http::verb::head) {
        std::string target = req.target().substr(1); // Убираем ведущий '/'
        std::string body = "Hello, " + target;
        res.result(http::status::ok);
        res.set(http::field::content_type, "text/html");
        res.body() = body;

        // Если это не HEAD-запрос, добавляем тело в ответ
        if (req.method() == http::verb::get) {
            res.set(http::field::content_length, std::to_string(body.size()));
        }
    } 
    // Обработка других типов запросов
    else {
        res.result(http::status::method_not_allowed);
        res.set(http::field::content_type, "text/html");
        res.set(http::field::allow, "GET, HEAD");
        res.body() = "Invalid method";
        res.set(http::field::content_length, "14");
    }

    // Отправляем ответ
    beast::http::write(stream, res);
}

int main() {
    try {
        net::io_context ioc;
        tcp::acceptor acceptor(ioc, {net::ip::make_address("127.0.0.1"), 8080});
        std::cout << "Server has started..." << std::endl;

        while (true) {
            tcp::socket socket(ioc);
            acceptor.accept(socket);
            beast::tcp_stream stream(std::move(socket));

            beast::http::request<beast::http::string_body> req;
            beast::http::read(stream, beast::flat_buffer(), req);

            handle_request(stream, req);

            stream.socket().shutdown(tcp::socket::shutdown_send);
        }
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
    }

    return 0;
}


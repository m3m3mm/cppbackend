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

// Функция для формирования ответа на запрос
template <typename Body, typename Allocator>
http::response<http::string_body> HandleRequest(const http::request<Body, http::basic_fields<Allocator>>& req) {
    // Извлекаем target (адрес запроса)
    std::string target = std::string(req.target());
    
    // Удаляем ведущий символ '/' из адреса
    if (!target.empty() && target[0] == '/') {
        target = target.substr(1);
    }
    
    // Формируем тело ответа
    std::string body = "Hello, " + target;
    
    // Проверяем метод запроса
    if (req.method() == http::verb::get || req.method() == http::verb::head) {
        // Формируем ответ с кодом 200 OK
        http::response<http::string_body> response(http::status::ok, req.version());
        response.set(http::field::content_type, "text/html");
        response.set(http::field::content_length, std::to_string(body.size()));
        
        // Если метод HEAD, то не добавляем тело ответа
        if (req.method() == http::verb::get) {
            response.body() = body;
        }
        
        return response;
    } else {
        // Для всех остальных методов возвращаем статус 405 Method Not Allowed
        http::response<http::string_body> response(http::status::method_not_allowed, req.version());
        response.set(http::field::content_type, "text/html");
        response.set(http::field::allow, "GET, HEAD");
        
        // Устанавливаем тело ответа
        std::string error_body = "Invalid method";
        response.set(http::field::content_length, std::to_string(error_body.size()));
        response.body() = error_body;
        
        return response;
    }
}

int main() {
    try {
        // IP-адрес и порт, на котором будет работать сервер
        const auto address = net::ip::make_address("0.0.0.0");
        const unsigned short port = 8080;
        
        // Создаём контекст ввода-вывода
        net::io_context ioc;
        
        // Создаём акцептор для приёма входящих соединений
        tcp::acceptor acceptor(ioc, {address, port});
        
        // Выводим сообщение о готовности сервера
        std::cout << "Server has started..."sv << std::endl;
        
        while (true) {
            // Создаём сокет для клиента
            tcp::socket socket(ioc);
            
            // Принимаем подключение
            acceptor.accept(socket);
            
            // Буфер для чтения данных
            beast::flat_buffer buffer;
            
            // Создаём парсер запроса
            http::request<http::string_body> request;
            
            // Считываем запрос
            http::read(socket, buffer, request);
            
            // Обрабатываем запрос и получаем ответ
            auto response = HandleRequest(request);
            
            // Отправляем ответ клиенту
            http::write(socket, response);
            
            // Закрываем соединение
            beast::error_code ec;
            socket.shutdown(tcp::socket::shutdown_send, ec);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

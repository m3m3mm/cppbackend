// src/request_handler.h
#pragma once
#include "http_server.h" // Для http::request, http::response и псевдонима http_server::RequestHandler (std::function)
#include "model.h"
#include <functional>
#include <boost/json.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
namespace fs = std::filesystem;

// Конкретный тип колбэка, используемый методами этого обработчика для отправки ответов
using StringResponseSendCallback = std::function<void(http::response<http::string_body>&&)>;

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game, const std::string& static_files_dir)
        : game_{game}
        , static_files_dir_{static_files_dir} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    // operator() шаблонизирован для приёма конкретного типа колбэка отправки из http_server
    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send_cb) {
        
        const auto& target = req.target(); // Константная ссылка на target
        const auto method = req.method();
        unsigned http_version = req.version();
        bool keep_alive = req.keep_alive();

        // Адаптируем общий Send&& send_cb к конкретному StringResponseSendCallback.
        // Это позволяет HandleGetMaps/HandleGetMap иметь конкретную сигнатуру.
        StringResponseSendCallback sender = std::forward<Send>(send_cb);

        // Предполагаем, что Body - это http::string_body, как используется в http_server.cpp.
        // В данном проекте http_server использует http::string_body.
        // Создаем константную ссылку на конкретный тип запроса, чтобы передавать ее дальше
        // и избежать проблем с перемещением из req, если он нужен в нескольких ветках.
        // Это важно, так как req (параметр) может быть rvalue-ссылкой, и мы не хотим его перемещать досрочно.
        const auto& concrete_req_ref = static_cast<const http::request<http::string_body>&>(req);

        // Normalize target to always start with a slash
        std::string normalized_target = target.starts_with('/') ? std::string(target) : "/" + std::string(target);
        beast::string_view norm_target_sv{normalized_target};

        // Define the API prefix for map IDs
        static const beast::string_view API_PREFIX = "/api/";
        static const beast::string_view API_MAP_PREFIX = "/api/v1/maps/";
        static const beast::string_view API_MAPS = "/api/v1/maps";

        if (norm_target_sv.starts_with(API_PREFIX)) {
            // Handle API requests
            if (norm_target_sv == API_MAPS) {
                if (method == http::verb::get) {
                    HandleGetMaps(concrete_req_ref, sender);
                } else {
                    sender(MakeErrorResponse(http::status::method_not_allowed, "methodNotAllowed", "Method not allowed", http_version, keep_alive, false));
                }
            } else if (norm_target_sv.starts_with(API_MAP_PREFIX)) {
                // Проверяем, что после префикса есть хотя бы один символ (ID карты не пустой)
                if (norm_target_sv.size() > API_MAP_PREFIX.size()) {
                    if (method == http::verb::get) {
                        auto id_sv = norm_target_sv.substr(API_MAP_PREFIX.size());
                        // Trim leading/trailing slashes
                        while (!id_sv.empty() && id_sv.front() == '/') id_sv.remove_prefix(1);
                        while (!id_sv.empty() && id_sv.back() == '/') id_sv.remove_suffix(1);
                        HandleGetMap(concrete_req_ref, sender, id_sv);
                    } else {
                        sender(MakeErrorResponse(http::status::method_not_allowed, "methodNotAllowed", "Method not allowed", http_version, keep_alive, false));
                    }
                } else {
                    sender(MakeErrorResponse(http::status::bad_request, "badRequest", "Bad request", http_version, keep_alive, false));
                }
            } else {
                sender(MakeErrorResponse(http::status::bad_request, "badRequest", "Bad request", http_version, keep_alive, false));
            }
        } else {
            // Handle static file requests
            if (method == http::verb::get || method == http::verb::head) {
                HandleStaticFile(concrete_req_ref, sender, method == http::verb::head);
            } else {
                sender(MakeErrorResponse(http::status::method_not_allowed, "methodNotAllowed", "Method not allowed", http_version, keep_alive, false));
            }
        }
    }

private:
    // Вспомогательные методы теперь принимают константную ссылку на запрос и колбэк отправки
    void HandleGetMaps(const http::request<http::string_body>& req, StringResponseSendCallback& sender);
    
    void HandleGetMap(const http::request<http::string_body>& req, StringResponseSendCallback& sender, beast::string_view id_sv);

    void HandleStaticFile(const http::request<http::string_body>& req, StringResponseSendCallback& sender, bool head_only);

    // Эти вспомогательные функции также нуждаются в версии HTTP и флаге keep_alive из запроса
    http::response<http::string_body> MakeErrorResponse(
        http::status status, beast::string_view code, beast::string_view message,
        unsigned http_version, bool keep_alive, bool plain_text);
    
    http::response<http::string_body> MakeSuccessResponse(
        http::status status, const json::value& body,
        unsigned http_version, bool keep_alive);

    std::string GetMimeType(const std::string& path);
    std::string UrlDecode(const std::string& str);
    bool IsPathTraversal(const std::string& path);

    model::Game& game_; // Ссылка на модель игры
    std::string static_files_dir_;
    std::unordered_map<std::string, std::string> mime_types_{
        {".htm", "text/html"},
        {".html", "text/html"},
        {".css", "text/css"},
        {".txt", "text/plain"},
        {".js", "text/javascript"},
        {".json", "application/json"},
        {".xml", "application/xml"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpe", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".bmp", "image/bmp"},
        {".ico", "image/vnd.microsoft.icon"},
        {".tiff", "image/tiff"},
        {".tif", "image/tiff"},
        {".svg", "image/svg+xml"},
        {".svgz", "image/svg+xml"},
        {".mp3", "audio/mpeg"}
    };
};

} // namespace http_handler

#pragma once

// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/json.hpp>

#include <filesystem>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>
#include <unordered_set>

#include "http_server.h"
#include "model.h"
#include "target_storage.h"
#include "body_constructor.h"


namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace fs = std::filesystem;
namespace sys = boost::system;

using namespace std::literals;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;
// Ответ, тело которого представлено в виде файла
using FileResponse = http::response<http::file_body>;

using BodyResponceVariant = std::variant<std::monostate, http::response<http::string_body>, http::response<http::file_body>>;

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html";
    constexpr static std::string_view TEXT_CSS = "text/css";
    constexpr static std::string_view TEXT_PLAIN = "text/plain";
    constexpr static std::string_view TEXT_JAVASCRIPT = "text/javascript";
    constexpr static std::string_view APPLICATION_JSON = "application/json";
    constexpr static std::string_view APPLICATION_XML = "application/xml";
    constexpr static std::string_view APPLICATION_OCTET = "application/octet-stream";
    constexpr static std::string_view IMAGE_PNG = "image/png";
    constexpr static std::string_view IMAGE_JPEG = "image/jpeg";
    constexpr static std::string_view IMAGE_GIF = "image/gif";
    constexpr static std::string_view IMAGE_BMP = "image/bmp";
    constexpr static std::string_view IMAGE_ICO = "image/vnd.microsoft.icon";
    constexpr static std::string_view IMAGE_TIFF = "image/tiff";
    constexpr static std::string_view IMAGE_SVG = "image/svg+xml";
    constexpr static std::string_view AUDIO_MPEG = "audio/mpeg";    
    // При необходимости внутрь ContentType можно добавить и другие типы контента
};

static const std::unordered_map<std::string, std::string_view> ExtensionToContentType {
    {".html", ContentType::TEXT_HTML},
    {".htm", ContentType::TEXT_HTML},
    {".css", ContentType::TEXT_CSS},
    {".txt", ContentType::TEXT_PLAIN},
    {".js", ContentType::TEXT_JAVASCRIPT},
    {".json", ContentType::APPLICATION_JSON},
    {".xml", ContentType::APPLICATION_XML},
    {".png", ContentType::IMAGE_PNG},
    {".jpg", ContentType::IMAGE_JPEG},
    {".jpa", ContentType::IMAGE_JPEG},
    {".jpeg", ContentType::IMAGE_JPEG},
    {".gif", ContentType::IMAGE_GIF},
    {".bmp", ContentType::IMAGE_BMP},
    {".ico", ContentType::IMAGE_ICO},
    {".tiff", ContentType::IMAGE_TIFF},
    {".tif", ContentType::IMAGE_TIFF},
    {".svg", ContentType::IMAGE_SVG},
    {".svgz", ContentType::IMAGE_SVG},
    {".mp3", ContentType::AUDIO_MPEG}
};

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game, fs::path base_path)
        : game_{game},
          base_path_(fs::weakly_canonical(base_path)) {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        BodyResponceVariant responce = HandleRequest(std::move(req));
        
        if(std::holds_alternative<StringResponse>(responce)) {
            send(std::get<StringResponse>(responce));
        }
        if(std::holds_alternative<FileResponse>(responce)) {
            send(std::get<FileResponse>(responce));
        }
    }

private:
    model::Game& game_;
    fs::path base_path_;

    BodyResponceVariant HandleRequest(StringRequest&& req);

    BodyResponceVariant MakeResponse(const StringRequest& req);

    StringResponse MakeStringResponse(targets_storage::TargetRequestType req_type, const StringRequest& req);

    FileResponse MakeFileResponse(targets_storage::TargetRequestType req_type, const StringRequest& req,
                                  http::file_body::value_type&& file, std::string_view content_type);

    StringResponse MakeStringOtherResponse(http::status status, const StringRequest& req,
                                           std::string_view message,
                                           std::string_view content_type = ContentType::TEXT_PLAIN);

    fs::path DecodeURL(const fs::path& req_path);

    targets_storage::TargetRequestType ComputeRequestType(std::string_view target);
    std::string ComputeRequestedObject(std::string_view target);

    fs::path ComputeReqestedPath(const fs::path& req_path);
    std::string_view ComputeContentType(const fs::path& req_path);

    //Возвращает nullopt, если файл не удалось открыть, либо он не существует
    std::optional<http::file_body::value_type> ComputeExistingFile(const std::string& req_path);
    
    bool IsSubPath(const fs::path& req_path);
};
}  // namespace http_handler
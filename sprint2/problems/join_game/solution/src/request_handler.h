#pragma once

// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/json.hpp>

#include <chrono>
#include <filesystem>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>
#include <unordered_set>

#include "game_properties.h"
#include "http_server.h"
#include "json_constructor.h"
#include "model.h"
#include "logger.h"
#include "target_storage.h"
#include "player_properties.h"

namespace http_handler {
namespace beast = boost::beast;
namespace fs = std::filesystem;
namespace http = beast::http;
namespace net = boost::asio;
namespace sys = boost::system;

using namespace std::literals;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;
// Ответ, тело которого представлено в виде файла
using FileResponse = http::response<http::file_body>;

using BodyResponceVariant = std::variant<StringResponse, FileResponse>;
using Strand = net::strand<net::io_context::executor_type>;

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

class ApiHandler {
public:
    ApiHandler(const Strand& api_strand, model::Game& game) : api_strand_(api_strand)
                                                            , game_(game) {
    }
    
    StringResponse HanldeApiRequest(const StringRequest& req, targets_storage::TargetRequestType req_type);
    Strand GetApiStrand();
private:
    Strand api_strand_;
    model::Game& game_;
    player::Players players_;

    std::string ComputeRequestedObject(std::string_view target);

    player::AuthorizationInfo ProcessJoinGame(const player::JoiningInfo& info);
};

class RequestHandler : public std::enable_shared_from_this<RequestHandler> {
public:
    explicit RequestHandler(Strand api_strand,model::Game& game, fs::path base_path)
        : base_path_(fs::weakly_canonical(base_path)),
          api_handler_(api_strand, game) {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        using namespace targets_storage;
    
        auto decoded_req_path = DecodeURL(req.target()).string();                                 
        auto req_type = ComputeRequestType(decoded_req_path);

        auto iter = decoded_req_path.find_last_of("."s);

        if(iter != decoded_req_path.npos) {
            std::transform(decoded_req_path.begin() + iter + 1, decoded_req_path.end(), decoded_req_path.begin() + iter + 1,
                           [](unsigned char c){ 
                               return std::tolower(c); 
                           });
        }

        if(ValidateCompability(req.method(), req_type)) {
            switch (req_type) {
                case TargetRequestType::GET_FILE : {
                    std::visit(
                        [&send](auto&& result) {
                            send(std::forward<decltype(result)>(result));
                        },
                        HandleFileRequest(req, decoded_req_path));
                    break;
                }
                
                //Будем считать, что по умолчанию /api
                default : {
                    auto handle = [self = shared_from_this(), send,
                                   req = std::forward<decltype(req)>(req), req_type] {
                        assert(self->api_handler_.GetApiStrand().running_in_this_thread());
                        return send(self->api_handler_.HanldeApiRequest(req, req_type));
                    };

                    net::dispatch(api_handler_.GetApiStrand(), handle);
                    break;
                }
            }
        } else {
            send(HandleErrorRequest(req, req_type));
        }
    }

private:
    fs::path base_path_;
    ApiHandler api_handler_;

    BodyResponceVariant HandleFileRequest(const StringRequest& req, const fs::path& req_path);
    StringResponse HandleErrorRequest(const StringRequest& req, targets_storage::TargetRequestType req_type);

    FileResponse MakeFileResponse(const StringRequest& req, http::file_body::value_type&& file, std::string_view content_type,
                                  http::status status);

    StringResponse MakeStringOtherResponse(http::status status, const StringRequest& req,
                                           std::string_view message,
                                           std::string_view content_type = ContentType::TEXT_PLAIN,
                                           std::string_view allowed_methods = "");

    fs::path DecodeURL(const fs::path& req_path);

    targets_storage::TargetRequestType ComputeRequestType(std::string_view target);
    fs::path ComputeReqestedPath(const fs::path& req_path);
    std::string_view ComputeContentType(const fs::path& req_path);

    //Возвращает nullopt, если файл не удалось открыть, либо он не существует
    std::optional<http::file_body::value_type> ComputeExistingFile(const std::string& req_path);
    
    bool IsSubPath(const fs::path& req_path);
    bool ValidateCompability(http::verb method, targets_storage::TargetRequestType req_type);
};
}  // namespace http_handler
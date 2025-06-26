#pragma once

// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/beast/http.hpp>

#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace targets_storage {
namespace beast = boost::beast; 
namespace http = beast::http;

enum class TargetRequestType {
    GET_MAPS_INFO,
    GET_MAP_BY_ID,
    GET_FILE,
    GET_PLAYERS,
    GET_STATE,
    GET_RECORDS,
    POST_JOIN_GAME,
    POST_ACTION,
    POST_TICK,
    ERROR_API,
    UNKNOW
};

struct UsingTargetPath {
    UsingTargetPath() = delete;
    constexpr static std::string_view RECORDS = "/api/v1/game/records";
    constexpr static std::string_view TICK = "/api/v1/game/tick";
    constexpr static std::string_view ACTION = "/api/v1/game/player/action";
    constexpr static std::string_view STATE = "/api/v1/game/state";
    constexpr static std::string_view JOIN = "/api/v1/game/join";
    constexpr static std::string_view MAPS = "/api/v1/maps";
    constexpr static std::string_view PLAYERS = "/api/v1/game/players";
    constexpr static std::string_view API = "/api";
};

struct TargetErrorMessage {
    TargetErrorMessage() = delete;
    constexpr static std::string_view ERROR_SEARCH_MAP_MESSAGE = "Map not found: ";
    constexpr static std::string_view ERROR_SEARCH_FILE_MESSAGE = "File not found";
    constexpr static std::string_view ERROR_SEARCH_TOKEN_MESSAGE = "Player token has not been found";
    constexpr static std::string_view ERROR_INVALID_PATH_MESSAGE = "Invalide path: ";
    constexpr static std::string_view ERROR_INVALID_TOKEN_MESSAGE = "Authorization header is required";
    constexpr static std::string_view ERROR_INVALID_METHOD_MESSAGE = "Invalid method";
    constexpr static std::string_view ERROR_INVALID_NAME_MESSAGE = "Invalid name";
    constexpr static std::string_view ERROR_INVALID_CONTENT_MESSAGE = "Invalid content type";
    constexpr static std::string_view ERROR_INVALID_JOIN_PARSE_MESSAGE = "Join game request parse error";
    constexpr static std::string_view ERROR_INVALID_ACTION_PARSE_MESSAGE = "Failed to parse action";
    constexpr static std::string_view ERROR_INVALID_TICK_PARSE_MESSAGE = "Failed to parse tick request JSON";
    constexpr static std::string_view ERROR_BAD_REQUEST_MESSAGE = "Bad request";
};

struct TargetErrorCode {
    TargetErrorCode() = delete;
    constexpr static std::string_view ERROR_SEARCH_MAP_CODE = "mapNotFound";
    constexpr static std::string_view ERROR_SEARCH_TOKEN_CODE = "unknownToken";
    constexpr static std::string_view ERROR_INVALID_ARGUMENT_CODE = "invalidArgument";
    constexpr static std::string_view ERROR_INVALID_TOKEN_CODE = "invalidToken";
    constexpr static std::string_view ERROR_INVALID_METHOD_CODE = "invalidMethod";
    constexpr static std::string_view ERROR_BAD_REQUEST_CODE = "badRequest";
};

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

static std::unordered_map<std::string_view, std::vector<http::verb>> allowed_methods {
    {UsingTargetPath::MAPS,    {http::verb::get,
                                http::verb::head}},
    {UsingTargetPath::PLAYERS, {http::verb::get,
                                http::verb::head}},
    {UsingTargetPath::STATE,   {http::verb::get,
                                http::verb::head}},
    {UsingTargetPath::RECORDS, {http::verb::get,
                                http::verb::head}},
    {UsingTargetPath::JOIN,    {http::verb::post}},
    {UsingTargetPath::ACTION,  {http::verb::post}},
    {UsingTargetPath::TICK,    {http::verb::post}}

};

static std::unordered_map<http::verb, std::unordered_set<TargetRequestType>> valid_compability {
    {http::verb::get, {TargetRequestType::GET_MAPS_INFO,
                       TargetRequestType::GET_MAP_BY_ID,
                       TargetRequestType::GET_FILE,
                       TargetRequestType::GET_PLAYERS,
                       TargetRequestType::GET_STATE,
                       TargetRequestType::GET_RECORDS,
                       TargetRequestType::ERROR_API,
                      }
    },
    {http::verb::head, {TargetRequestType::GET_MAPS_INFO,
                        TargetRequestType::GET_MAP_BY_ID,
                        TargetRequestType::GET_FILE,
                        TargetRequestType::GET_PLAYERS,
                        TargetRequestType::GET_STATE,
                        TargetRequestType::GET_RECORDS,
                        TargetRequestType::ERROR_API,
                       }

    },
    {http::verb::post, {TargetRequestType::POST_JOIN_GAME,
                        TargetRequestType::POST_ACTION,
                        TargetRequestType::POST_TICK
                       }

    }
};
}//namespace targets_storage
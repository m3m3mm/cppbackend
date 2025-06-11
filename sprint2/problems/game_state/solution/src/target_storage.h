#pragma once

// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/beast/http.hpp>

#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace targets_storage {
namespace beast = boost::beast; 
namespace http = beast::http;

enum class TargetRequestType {
    GET_MAPS_INFO,
    GET_MAP_BY_ID,
    GET_FILE,
    GET_PLAYERS,
    GET_STATE,
    POST_JOIN_GAME,
    ERROR_API,
    UNKNOW
};

struct UsingTargetPath {
    UsingTargetPath() = delete;
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
    constexpr static std::string_view ERROR_INVALID_ARGUMENT_MESSAGE = "Join game request parse error";
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

static std::unordered_map<std::string_view, std::string_view> allowed_methods {
    {UsingTargetPath::MAPS, "GET, HEAD"},
    {UsingTargetPath::JOIN, "POST"},
    {UsingTargetPath::PLAYERS, "GET, HEAD"},
    {UsingTargetPath::STATE, "GET, HEAD"}
};

static std::unordered_map<http::verb, std::unordered_set<TargetRequestType>> valid_compability {
    {http::verb::get, {TargetRequestType::GET_MAPS_INFO,
                       TargetRequestType::GET_MAP_BY_ID,
                       TargetRequestType::GET_FILE,
                       TargetRequestType::GET_PLAYERS,
                       TargetRequestType::GET_STATE,
                       TargetRequestType::ERROR_API,
                      }
    },
    {http::verb::head, {TargetRequestType::GET_MAPS_INFO,
                        TargetRequestType::GET_MAP_BY_ID,
                        TargetRequestType::GET_FILE,
                        TargetRequestType::GET_PLAYERS,
                        TargetRequestType::GET_STATE,
                        TargetRequestType::ERROR_API,
                       }

    },
    {http::verb::post, {TargetRequestType::POST_JOIN_GAME,
                       }

    }
};
}//namespace targets_storage
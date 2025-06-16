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
    ERROR_API,
    UNKNOW
};

struct UsingTargetPath {
    UsingTargetPath() = delete;
    constexpr static std::string_view MAPS = "/api/v1/maps";
    constexpr static std::string_view API = "/api";
};

struct TargetErrorMessage {
    TargetErrorMessage() = delete;
    constexpr static std::string_view ERROR_SEARCH_MAP_MESSAGE = "Map not found: ";
    constexpr static std::string_view ERROR_SEARCH_FILE_MESSAGE = "File not found";
    constexpr static std::string_view ERROR_INVALID_PATH_MESSAGE = "Invalide path: ";
    constexpr static std::string_view ERROR_INVALID_METHOD_MESSAGE = "Invalid method";
    constexpr static std::string_view ERROR_BAD_REQUEST_MESSAGE = "Bad request";
};

struct TargetErrorCode {
    TargetErrorCode() = delete;
    constexpr static std::string_view ERROR_SEARCH_MAP_CODE = "mapNotFound";
    constexpr static std::string_view ERROR_BAD_REQUEST_CODE = "badRequest";
};

static std::unordered_map<http::verb, std::unordered_set<TargetRequestType>> valid_compability {
    {http::verb::get, {TargetRequestType::GET_MAPS_INFO,
                       TargetRequestType::GET_MAP_BY_ID,
                       TargetRequestType::GET_FILE,
                       TargetRequestType::ERROR_API
                      }
    },
    {http::verb::head, {TargetRequestType::GET_MAPS_INFO,
                        TargetRequestType::GET_MAP_BY_ID,
                        TargetRequestType::GET_FILE,
                        TargetRequestType::ERROR_API
                       }

    }
};
}//namespace targets_storage
#pragma once

#include <string_view>

namespace targets_storage {
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
}//namespace targets_storage
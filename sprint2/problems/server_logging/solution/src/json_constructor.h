#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "logger.h"
#include "model.h"
#include "target_storage.h"

namespace json_constructor {
    std::string MakeBodyJSON(targets_storage::TargetRequestType req_type,
                             const  model::Game& game = model::Game(),
                             const std::string& req_object = "");
    
    boost::json::object MakeLogStartJSON(int port, const std::string& address);
    boost::json::object MakeLogStopJSON(int code, const std::optional<std::string>& exception = {});
    boost::json::object MakeLogRequestJSON(const std::string& ip, const std::string& uri, const std::string& method);
    boost::json::object MakeLogResponceJSON(int response_time, int code, std::string_view content_type);
    boost::json::object MakeLogErrorJSON(int code, const std::string& text, const std::string& where);
}
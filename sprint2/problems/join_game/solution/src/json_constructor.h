#pragma once

#include <boost/json.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <deque>

#include "game_properties.h"
#include "logger.h"
#include "model.h"
#include "target_storage.h"
#include "json_tags.h"
#include "player_properties.h"

namespace json_constructor {
std::string MakeBodyJSON(targets_storage::TargetRequestType req_type,
                         const model::Game& game = model::Game(),
                         const std::string& req_object = "");

std::string MakeBodyJSON(targets_storage::TargetRequestType req_type, const player::AuthorizationInfo& object);

std::string MakeBodyJSON(targets_storage::TargetRequestType req_type, const std::vector<model::Dog>& dogs);

std::string MakeBodyErrorJSON(std::string_view error_code,
                              std::string_view error_message, 
                              const std::string& req_object = ""); 
    
boost::json::object MakeLogStartJSON(int port, const std::string& address);
boost::json::object MakeLogStopJSON(int code, const std::optional<std::string>& exception = {});
boost::json::object MakeLogRequestJSON(const std::string& ip, const std::string& uri, const std::string& method);
boost::json::object MakeLogResponceJSON(int response_time, int code, std::string_view content_type);
boost::json::object MakeLogErrorJSON(int code, const std::string& text, const std::string& where);
}
#pragma once

#include <boost/json.hpp>

#include <list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "../app/player_properties.h"
#include "../handlers/target_storage.h"
#include "../model/dynamic_object_properties.h"
#include "../model/game_properties.h"
#include "../model/static_object_prorerties.h"
#include "json_tags.h"

namespace json_constructor {
std::string MakeBodyJSON(targets_storage::TargetRequestType req_type,
                         const model::Game& game = model::Game(),
                         const std::string& req_object = "",
                         const boost::json::array* types = nullptr);
std::string MakeBodyJSON(const player::AuthorizationInfo& object);
std::string MakeBodyJSON(const std::vector<std::shared_ptr<model::Dog>>& dogs);
std::string MakeBodyJSON(const model::GameState& state);
std::string MakeBodyJSON(const std::vector<player::PlayerRecord>& records);

std::string MakeBodyErrorJSON(std::string_view error_code,
                              std::string_view error_message, 
                              const std::string& req_object = "");
std::string MakeBodyEmptyObject(); 
    
boost::json::object MakeLogStartJSON(int port, const std::string& address);
boost::json::object MakeLogStopJSON(int code, const std::optional<std::string>& exception = {});
boost::json::object MakeLogRequestJSON(const std::string& ip, const std::string& uri, const std::string& method);
boost::json::object MakeLogResponceJSON(int response_time, int code, std::string_view content_type);
boost::json::object MakeLogErrorJSON(int code, const std::string& text, const std::string& where);
}
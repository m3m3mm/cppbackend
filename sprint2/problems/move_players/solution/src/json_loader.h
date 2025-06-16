#pragma once

#include <boost/json/parse.hpp>
#include <boost/json/serialize.hpp>

#include <filesystem>
#include <fstream>
#include <optional>

#include "game_properties.h"
#include "model.h"
#include "player_properties.h"

namespace json_loader {
model::Game LoadGame(const std::filesystem::path& json_path);
std::optional<player::JoiningInfo> LoadJoiningInfo(const std::string& req_post);
std::optional<model::Direction> LoadMoveInfo(const std::string& req_post);
}  // namespace json_loader

#pragma once

#include <boost/json/parse.hpp>
#include <boost/json/serialize.hpp>

#include <filesystem>
#include <fstream>
#include <optional>

#include "../app/player_properties.h"
#include "../model/game_properties.h"
#include "../model/static_object_prorerties.h"
#include "../server/extra_data.h"

namespace json_loader {
model::Game LoadGame(const std::filesystem::path& json_path, extra_data::LootTypes& loot_types, bool is_random_spawn);
std::optional<player::JoiningInfo> LoadJoiningInfo(const std::string& req_post);
std::optional<model::Direction> LoadUpdateInfo(const std::string& req_post);
std::optional<int64_t> LoadTickInfo(const std::string& req_post);
}  // namespace json_loader

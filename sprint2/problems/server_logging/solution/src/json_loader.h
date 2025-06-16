#pragma once

#include <filesystem>
#include <fstream>

#include "model.h"

namespace json_loader {
model::Game LoadGame(const std::filesystem::path& json_path);
}  // namespace json_loader

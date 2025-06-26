#pragma once

#include <boost/program_options.hpp>

#include <filesystem>
#include <optional>
#include <string>

namespace fs = std::filesystem;

namespace command_handler {
struct Args {
    int tick_period = 0;
    int save_state_period = 0;
    bool randomize_spawn_point = false;
    fs::path config_file = "";
    fs::path static_dir = "";
    fs::path state_file = "";
};

std::optional<Args> HandleCommands(int argc, const char* const argv[]);
} // namespace command_handler
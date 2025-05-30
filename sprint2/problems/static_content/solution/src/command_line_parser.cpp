#include "command_line_parser.h"

#include <iostream>
#include <stdexcept>

using namespace std::literals;

namespace command_line {

std::optional<Args> ParseCommandLine(int argc, const char *const argv[])
{
    Args parsed_args;

    namespace po = boost::program_options;
    po::options_description options_desc("Allowed options"s);
    options_desc.add_options()
        ("help,h", "produce help message")
        ("tick-period,t", po::value(&parsed_args.tick_period)->value_name("milliseconds"s), "set tick period")
        ("config-file,c", po::value(&parsed_args.config_file)->value_name("file"s), "set config file path")
        ("www-root,w", po::value(&parsed_args.source_dir)->value_name("dir"s), "set static files root")
        ("randomize-spawn-points", po::bool_switch(&parsed_args.randomize_spawn_point), "spawn dogs at random positions");
    
    po::variables_map variables;
    po::store(po::parse_command_line(argc, argv, options_desc), variables);
    po::notify(variables);

    if (variables.contains("help"s)) {
        std::cout << options_desc;
        return std::nullopt;
    }

    if (!variables.contains("config-file"s)) {
        throw std::runtime_error("Config file isn't set!");
    }

    if (!variables.contains("www-root"s)) {
        throw std::runtime_error("Static files directory isn't set!");
    }

    return parsed_args;
}

}
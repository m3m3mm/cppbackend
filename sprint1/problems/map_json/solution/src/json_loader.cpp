#include "json_loader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <boost/json.hpp>

namespace json_loader {

namespace json = boost::json;

model::Game LoadGame(const std::filesystem::path& json_path) {
    // Read file content
    std::ifstream file(json_path);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + json_path.string());
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    // Parse JSON
    auto json = boost::json::parse(content);
    auto& maps = json.as_object().at("maps").as_array();
    
    model::Game game;
    
    for (const auto& map_json : maps) {
        std::string map_id = std::string(map_json.at("id").as_string());
        std::string map_name = std::string(map_json.at("name").as_string());
        std::cerr << "Loading map: id='" << map_id << "', name='" << map_name << "'" << std::endl;
        
        model::Map map(
            model::Map::Id{map_id},
            map_name
        );
        
        // Add roads
        for (const auto& road_json : map_json.at("roads").as_array()) {
            auto& road_obj = road_json.as_object();
            int x0 = static_cast<int>(road_obj.at("x0").as_int64());
            int y0 = static_cast<int>(road_obj.at("y0").as_int64());
            
            if (road_obj.contains("x1")) {
                // Horizontal road
                int x1 = static_cast<int>(road_obj.at("x1").as_int64());
                std::cerr << "  Adding horizontal road: (" << x0 << "," << y0 << ") -> (" << x1 << "," << y0 << ")\n";
                map.AddRoad(model::Road(
                    model::Road::HORIZONTAL,
                    model::Point{x0, y0},
                    x1
                ));
            } else {
                // Vertical road
                int y1 = static_cast<int>(road_obj.at("y1").as_int64());
                std::cerr << "  Adding vertical road: (" << x0 << "," << y0 << ") -> (" << x0 << "," << y1 << ")\n";
                map.AddRoad(model::Road(
                    model::Road::VERTICAL,
                    model::Point{x0, y0},
                    y1
                ));
            }
        }
        
        // Add buildings
        auto& map_obj = map_json.as_object();
        if (map_obj.contains("buildings")) {
            for (const auto& building_json : map_obj.at("buildings").as_array()) {
                auto& building_obj = building_json.as_object();
                int x = static_cast<int>(building_obj.at("x").as_int64());
                int y = static_cast<int>(building_obj.at("y").as_int64());
                int w = static_cast<int>(building_obj.at("w").as_int64());
                int h = static_cast<int>(building_obj.at("h").as_int64());
                std::cerr << "  Adding building: (" << x << "," << y << ") " << w << "x" << h << "\n";
                map.AddBuilding(model::Building(model::Rectangle{
                    model::Point{x, y},
                    model::Size{w, h}
                }));
            }
        }
        
        // Add offices
        if (map_obj.contains("offices")) {
            for (const auto& office_json : map_obj.at("offices").as_array()) {
                auto& office_obj = office_json.as_object();
                std::string office_id = std::string(office_obj.at("id").as_string());
                int x = static_cast<int>(office_obj.at("x").as_int64());
                int y = static_cast<int>(office_obj.at("y").as_int64());
                int offset_x = static_cast<int>(office_obj.at("offsetX").as_int64());
                int offset_y = static_cast<int>(office_obj.at("offsetY").as_int64());
                std::cerr << "  Adding office: id='" << office_id << "', pos=(" << x << "," << y << "), offset=(" << offset_x << "," << offset_y << ")\n";
                map.AddOffice(model::Office(
                    model::Office::Id{office_id},
                    model::Point{x, y},
                    model::Offset{offset_x, offset_y}
                ));
            }
        }
        
        game.AddMap(std::move(map));
    }
    
    return game;
}

}  // namespace json_loader

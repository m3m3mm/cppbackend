#include "json_loader.h"
#include <fstream>
#include <sstream>
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
        auto& map_obj = map_json.as_object();
        
        // Create map
        model::Map map(
            model::Map::Id{std::string(map_obj.at("id").as_string())},
            std::string(map_obj.at("name").as_string())
        );
        
        // Add roads
        for (const auto& road_json : map_obj.at("roads").as_array()) {
            auto& road_obj = road_json.as_object();
            int x0 = static_cast<int>(road_obj.at("x0").as_int64());
            int y0 = static_cast<int>(road_obj.at("y0").as_int64());
            
            if (road_obj.contains("x1")) {
                // Horizontal road
                map.AddRoad(model::Road(
                    model::Road::HORIZONTAL,
                    model::Point{x0, y0},
                    static_cast<int>(road_obj.at("x1").as_int64())
                ));
            } else {
                // Vertical road
                map.AddRoad(model::Road(
                    model::Road::VERTICAL,
                    model::Point{x0, y0},
                    static_cast<int>(road_obj.at("y1").as_int64())
                ));
            }
        }
        
        // Add buildings
        if (map_obj.contains("buildings")) {
            for (const auto& building_json : map_obj.at("buildings").as_array()) {
                auto& building_obj = building_json.as_object();
                map.AddBuilding(model::Building(model::Rectangle{
                    model::Point{
                        static_cast<int>(building_obj.at("x").as_int64()),
                        static_cast<int>(building_obj.at("y").as_int64())
                    },
                    model::Size{
                        static_cast<int>(building_obj.at("w").as_int64()),
                        static_cast<int>(building_obj.at("h").as_int64())
                    }
                }));
            }
        }
        
        // Add offices
        if (map_obj.contains("offices")) {
            for (const auto& office_json : map_obj.at("offices").as_array()) {
                auto& office_obj = office_json.as_object();
                map.AddOffice(model::Office(
                    model::Office::Id{std::string(office_obj.at("id").as_string())},
                    model::Point{
                        static_cast<int>(office_obj.at("x").as_int64()),
                        static_cast<int>(office_obj.at("y").as_int64())
                    },
                    model::Offset{
                        static_cast<int>(office_obj.at("offsetX").as_int64()),
                        static_cast<int>(office_obj.at("offsetY").as_int64())
                    }
                ));
            }
        }
        
        game.AddMap(std::move(map));
    }
    
    return game;
}

}  // namespace json_loader

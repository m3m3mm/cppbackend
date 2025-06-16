#include <boost/json/parse.hpp>
#include <boost/json/serialize.hpp>

#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_set>

#include "json_loader.h"

namespace json = boost::json;

using namespace std::literals;
using ObjJSON = json::object;
using ValueJSON = json::value;

namespace {
class InfrastructureLoadError : public std::invalid_argument {
public:
    enum class Infrastructure {
        BUILDING, 
        OFFICE,
        ROAD
    };

    InfrastructureLoadError(Infrastructure infrastructure,
                            std::string invalid_object /*строковое представление блока json файла*/,
                            std::string error_message = ""s/*Используется в случае, если необходимо вывести 
                                                            информацию из ранее обработанного исключения*/)
        : invalid_argument(MakeErrorMessage(infrastructure, invalid_object, error_message)) {
    };

private:
    static std::string MakeErrorMessage(Infrastructure infrastructure,
                                        std::string invalid_object,
                                        std::string error_message) {

        std::string message = invalid_object + ": "s + error_message;

        switch (infrastructure) {
        case Infrastructure::BUILDING:
            return "Error loading building: "s + message;
            break;

        case Infrastructure::OFFICE:
            return "Error loading office: "s + message;
            break;

        case Infrastructure::ROAD:
            return "Error loading road: "s + message;
            break;

        default:
            return "Unidentified error"s; 
            break;
        }    
    }
};

const ValueJSON* FindKey(const ObjJSON& obj_JSON, json::string_view key) {
    const auto value_ptr = obj_JSON.if_contains(key);

    if(!value_ptr) {
        json::string report("Not found key: ");
        report += key;
        throw std::runtime_error(std::string(report));
    }
    return value_ptr;
}

ValueJSON OpenJSON(const std::filesystem::path& json_path) {
    using namespace std;

    ifstream instr(json_path, ios::in | ios::binary);
    
    if(!instr) {
        throw runtime_error("Error opening file"s);
    }

    ostringstream game_info;
    boost::system::error_code ec;
    game_info << instr.rdbuf();
    auto JSON_value = boost::json::parse(game_info.str(), ec);
    return JSON_value;
}

using namespace model;
using Infrastructure = InfrastructureLoadError::Infrastructure;

Map PrepareMap(const ObjJSON& map_object) {
    Map::Id id_map(FindKey(map_object, "id")->as_string().data());
    std::string name_map(FindKey(map_object, "name")->as_string().data());

    return model::Map(id_map, name_map);
}

Road LoadRoad(const ObjJSON& road_object) {
    try {
        const auto& x0 = FindKey(road_object, "x0")->as_int64();
        const auto& y0 = FindKey(road_object, "y0")->as_int64(); 

        model::Point point({x0, y0});
        if(const auto& x1 = road_object.if_contains("x1")) {
            return Road(Road::HORIZONTAL,point, x1->as_int64());
        } else if( const auto& y1 = road_object.if_contains("y1")){
            return Road(Road::VERTICAL, point, y1->as_int64()); 
        } else {
            throw std::runtime_error("x1 or y1 not found"s);
        }
    } catch (const std::runtime_error err) {
        throw InfrastructureLoadError(Infrastructure::ROAD,
                                      json::serialize(road_object),
                                      err.what());
    }
}

Building LoadBulding(const ObjJSON& building_object) {
    try {
        int64_t x = FindKey(building_object, "x")->as_int64();
        int64_t y = FindKey(building_object, "y")->as_int64();
        int64_t w = FindKey(building_object, "w")->as_int64();
        int64_t h = FindKey(building_object, "h")->as_int64();

        return Building({model::Point(x,y),
                         model::Size(w, h)});
    } catch(const std::runtime_error err) {
        throw InfrastructureLoadError(Infrastructure::BUILDING,
                                      json::serialize(building_object),
                                      err.what());
    }
}

Office LoadOffice(const ObjJSON& office_object) {
    try {
        Office::Id id_office(FindKey(office_object, "id")->as_string().data());
        int64_t x = FindKey(office_object, "x")->as_int64();
        int64_t y = FindKey(office_object, "y")->as_int64();
        int64_t offsetX = FindKey(office_object, "offsetX")->as_int64();
        int64_t offsetY = FindKey(office_object, "offsetY")->as_int64();

        Point point(x, y);
        Offset offset(offsetX, offsetY);

        return Office(id_office, point,offset);
    } catch(const std::runtime_error err) {
        throw InfrastructureLoadError(Infrastructure::OFFICE,
                                      json::serialize(office_object),
                                      err.what());
    }
}

Map LoadMap(const ObjJSON& map_object) {
    Map map = PrepareMap(map_object);

    try {
        const auto* roads_in_map = FindKey(map_object, "roads");
        for(const auto& road : roads_in_map->as_array()) {
            map.AddRoad(LoadRoad(road.as_object()));
        }

        const auto* buildings_in_map = FindKey(map_object, "buildings");
        for(const auto& building : buildings_in_map->as_array()) {
            map.AddBuilding(LoadBulding(building.as_object()));
        }

        const auto* offices_in_map = FindKey(map_object, "offices");
        for(const auto& office: offices_in_map->as_array()) {
            map.AddOffice(LoadOffice(office.as_object()));
        }
    } catch(const InfrastructureLoadError& err) {
        std::string report = map.GetName() + ": " + err.what();
        throw std::invalid_argument(report);
    }

    return map;
};
} //namespace

namespace json_loader {

Game LoadGame(const std::filesystem::path& json_path) {
    using namespace std;

    Game game;

    const auto root = OpenJSON(json_path);

    const auto* maps_in_game = FindKey(root.as_object(), "maps");
    for(const auto& map : maps_in_game->as_array()) {
        game.AddMap(LoadMap(map.as_object()));
    }
               
    return game;
}
}  // namespace json_loader
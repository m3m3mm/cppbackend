#include <stdexcept>
#include <string>
#include <unordered_set>

#include "json_loader.h"
#include "json_tags.h"

namespace fs = std::filesystem;
namespace json = boost::json;

using namespace std::literals;
using namespace json_tag;

using ObjJSON = json::object;
using ValueJSON = json::value;
using std::string;
using std::invalid_argument;
using std::runtime_error;

namespace {
const static double DEFAULT_SPEED = 1.;

class InfrastructureLoadError : public invalid_argument {
public:
    enum class Infrastructure {
        BUILDING, 
        OFFICE,
        ROAD
    };

    InfrastructureLoadError(Infrastructure infrastructure,
                            string invalid_object /*строковое представление блока json файла*/,
                            string error_message = ""s/*Используется в случае, если необходимо вывести 
                                                      информацию из ранее обработанного исключения*/)
        : invalid_argument(MakeErrorMessage(infrastructure, invalid_object, error_message)) {
    };

private:
    static std::string MakeErrorMessage(Infrastructure infrastructure,
                                        string invalid_object,
                                        string error_message) {

        string message = invalid_object + ": "s + error_message;

        switch (infrastructure) {
        case Infrastructure::BUILDING:
            return ERROR_LOADING_BUILDING + message;
            break;

        case Infrastructure::OFFICE:
            return ERROR_LOADING_OFFICE + message;
            break;

        case Infrastructure::ROAD:
            return ERROR_LOADING_ROAD + message;
            break;

        default:
            return UNIDENTIFIED_ERROR; 
            break;
        }    
    }
};

const ValueJSON* FindKey(const ObjJSON& obj_JSON, json::string_view key) {
    const auto value_ptr = obj_JSON.if_contains(key);

    if(!value_ptr) {
        json::string report(ERROR_KEY_NOT_FOUND);
        report += key;
        throw runtime_error(string(report));
    }

    return value_ptr;
}

ValueJSON ParseJSON(const string& value) {
    return json::parse(value);
}

ValueJSON OpenJSON(const fs::path& json_path) {
    using namespace std;

    ifstream instr(json_path, ios::in | ios::binary);
    
    if(!instr) {
        throw runtime_error(ERROR_OPENING_FILE + json_path.string());
    }

    ostringstream game_info;
    
    game_info << instr.rdbuf();
    auto JSON_value = ParseJSON(game_info.str());

    return JSON_value;
}

using namespace model;
using Infrastructure = InfrastructureLoadError::Infrastructure;

Map PrepareMap(const ObjJSON& map_object) {
    Map::Id id_map(FindKey(map_object, ID)->as_string().data());
    string name_map(FindKey(map_object, NAME)->as_string().data());

    return Map(id_map, name_map);
}

double LoadSpeed(const ObjJSON& obj, double* speed = nullptr) {
    try {
        if(!speed){
            return FindKey(obj, DEFAULT_DOG_SPEED)->as_double();
        }

        return FindKey(obj, DOG_SPEED)->as_double();
        
    } catch(...) {
        if(!speed) {
            return DEFAULT_SPEED;
        }

        return *speed;
    }
}

Road LoadRoad(const ObjJSON& road_object) {
    try {
        const auto& x0 = FindKey(road_object, X0)->as_int64();
        const auto& y0 = FindKey(road_object, Y0)->as_int64(); 

        Point point({x0, y0});
        if(const auto& x1 = road_object.if_contains(X1)) {
            return Road(Road::HORIZONTAL,point, x1->as_int64());
        } else if( const auto& y1 = road_object.if_contains(Y1)){
            return Road(Road::VERTICAL, point, y1->as_int64()); 
        } else {
            throw std::runtime_error(ERROR_COORD_NOT_FOUND);
        }
    } catch (const std::runtime_error err) {
        throw InfrastructureLoadError(Infrastructure::ROAD,
                                      json::serialize(road_object),
                                      err.what());
    }
}

Building LoadBulding(const ObjJSON& building_object) {
    try {
        int64_t x = FindKey(building_object, X)->as_int64();
        int64_t y = FindKey(building_object, Y)->as_int64();
        int64_t w = FindKey(building_object, W)->as_int64();
        int64_t h = FindKey(building_object, H)->as_int64();

        return Building({Point(x,y),
                         Size(w, h)});
    } catch(const runtime_error err) {
        throw InfrastructureLoadError(Infrastructure::BUILDING,
                                      json::serialize(building_object),
                                      err.what());
    }
}

Office LoadOffice(const ObjJSON& office_object) {
    try {
        Office::Id id_office(FindKey(office_object, ID)->as_string().data());
        int64_t x = FindKey(office_object, X)->as_int64();
        int64_t y = FindKey(office_object, Y)->as_int64();
        int64_t offsetX = FindKey(office_object, OFFSET_X)->as_int64();
        int64_t offsetY = FindKey(office_object, OFFSET_Y)->as_int64();

        Point point(x, y);
        Offset offset(offsetX, offsetY);

        return Office(id_office, point,offset);
    } catch(const runtime_error err) {
        throw InfrastructureLoadError(Infrastructure::OFFICE,
                                      json::serialize(office_object),
                                      err.what());
    }
}

Map LoadMap(const ObjJSON& map_object, double speed) {
    Map map = PrepareMap(map_object);

    map.AddDogSpeed(LoadSpeed(map_object, &speed));

    try {
        const auto* roads_in_map = FindKey(map_object, ROADS);
        for(const auto& road : roads_in_map->as_array()) {
            map.AddRoad(LoadRoad(road.as_object()));
        }

        const auto* buildings_in_map = FindKey(map_object, BUILDINGS);
        for(const auto& building : buildings_in_map->as_array()) {
            map.AddBuilding(LoadBulding(building.as_object()));
        }

        const auto* offices_in_map = FindKey(map_object, OFFICES);
        for(const auto& office: offices_in_map->as_array()) {
            map.AddOffice(LoadOffice(office.as_object()));
        }
    } catch(const InfrastructureLoadError& err) {
        string report = map.GetName() + ": " + err.what();
        throw invalid_argument(report);
    }

    return map;
};
} //namespace

namespace json_loader {

Game LoadGame(const fs::path& json_path) {
    Game game;

    const auto root = OpenJSON(json_path);
    const auto* maps_in_game = FindKey(root.as_object(), MAPS);

    game.AddSpeed(LoadSpeed(root.as_object()));
    
    for(const auto& map : maps_in_game->as_array()) {
        game.AddMap(LoadMap(map.as_object(), game.GetSpeed()));
    }
               
    return game;
}
std::optional<player::JoiningInfo> LoadJoiningInfo(const string& req_post) {
    try {
        ValueJSON value = ParseJSON(req_post);

        string user_name = string(FindKey(value.as_object(), json_tag::USER_NAME)->as_string());  
        string map_id = string(FindKey(value.as_object(), json_tag::MAP_ID)->as_string());

        return player::JoiningInfo{user_name, Map::Id{map_id}};
    } catch(...) {
        return std::nullopt;
    }
    
}
std::optional<model::Direction> LoadMoveInfo(const std::string& req_post) {
    try {
        ValueJSON value = ParseJSON(req_post);

        string move_info = string(FindKey(value.as_object(), "move"s)->as_string());

        if(move_info.empty()){
            return Direction::STOP;
        } else if(move_info == "U") {
            return Direction::U;
        } else if( move_info == "D") {
            return Direction::D;
        } else if(move_info == "L") {
            return Direction::L;
        } else if(move_info == "R") {
            return Direction::R;
        } else{
            //Вернет nullopt, а значит ошибка загрузки запроса
            throw runtime_error("");
        }
    } catch(...) {
        return std::nullopt;
    }
}
} // namespace json_loader
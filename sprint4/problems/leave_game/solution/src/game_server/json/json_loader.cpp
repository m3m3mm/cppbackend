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

        case Infrastructure::OFFICE:
            return ERROR_LOADING_OFFICE + message;

        case Infrastructure::ROAD:
            return ERROR_LOADING_ROAD + message;

        default:
            return UNIDENTIFIED_ERROR; 
        }    
    }
};

void ThrowUnidentifiedError(const string& req_post) {
    string report =  UNIDENTIFIED_ERROR + req_post;
    throw runtime_error(report);
}

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

Map PrepareMap(const ObjJSON& map_object, extra_data::LootTypes& loot_types) {
    Map::Id id_map(FindKey(map_object, ID)->as_string().data());
    string name_map(FindKey(map_object, NAME)->as_string().data());

    const auto array_types = FindKey(map_object, LOOT_TYPES)->as_array();
    if(array_types.empty()) {
        throw runtime_error(ERROR_EMPTY_LOOT_TYPES);
    }
    loot_types.SetTypes(id_map, array_types);

    return Map(id_map, name_map);
}

double LoadSpeed(const ObjJSON& obj, const double* speed = nullptr) {
    try {
        if(!speed){
            return FindKey(obj, DEFAULT_DOG_SPEED)->as_double();
        }

        return FindKey(obj, DOG_SPEED)->as_double();
        
    } catch(runtime_error&) {
        if(!speed) {
            return model::DEFAULT_SPEED;
        }

        return *speed;
    }
}

size_t LoadBagCapacity(const ObjJSON& obj, const size_t* def_capacity = nullptr) {
    try {
        if(!def_capacity){
            return FindKey(obj, json_tag::DEFAULT_BAG_CAPACITY)->as_int64();
        }

        return FindKey(obj,BAG_CAPACITY)->as_int64();
        
    } catch(runtime_error&) {
        if(!def_capacity) {
            return model::DEFAULT_BAG_CAPACITY;
        }

        return *def_capacity;
    }
}

Road LoadRoad(const ObjJSON& road_object) {
    try {
        const auto& x0 = FindKey(road_object, X0)->as_int64();
        const auto& y0 = FindKey(road_object, Y0)->as_int64(); 

        Point point({x0, y0});
        if(const auto& x1 = road_object.if_contains(X1)) {
            return Road(Road::HORIZONTAL, point, x1->as_int64());
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

Map LoadMap(const ObjJSON& map_object, extra_data::LootTypes& loot_types, double speed, size_t capacity) {
    Map map = PrepareMap(map_object, loot_types);

    map.SetDogSpeed(LoadSpeed(map_object, &speed));
    map.SetBagCapacity(LoadBagCapacity(map_object, &capacity));
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

        const auto* types_array = loot_types.GetTypes(map.GetId());
        for(const auto& type : *types_array) {
            map.SerLootUnitCost(FindKey(type.as_object(),VALUE)->as_int64());
        }

    } catch(const InfrastructureLoadError& err) {
        string report = map.GetName() + ": " + err.what();
        throw invalid_argument(report);
    }

    return map;
}

LootGeneratorConfig LoadLootGenConfig(const json::object& loot_gen_conf) {
    double period = FindKey(loot_gen_conf, PERIOD)->as_double();
    double probability = FindKey(loot_gen_conf, PROBABILITY)->as_double();

    return LootGeneratorConfig{period, probability};
}

int64_t LoadRetirementTime(const json::object& root) {
    try { 
        return FindKey(root, DOG_RETIREMENT_TIME)->as_double() * model::MILLISECOND_PER_SECOND;
    } catch(const std::runtime_error&) {
        return model::RETIREMENT_TIME;
    }
}
} //namespace

namespace json_loader {

Game LoadGame(const fs::path& json_path, extra_data::LootTypes& loot_types,  bool is_random_spawn) {
    const auto root = OpenJSON(json_path);
    const auto* maps_in_game = FindKey(root.as_object(), MAPS);
    const auto* loot_generator_config = FindKey(root.as_object(), LOOT_GENERATOR_CONFIG);
    
    Game game(LoadLootGenConfig(loot_generator_config->as_object()), LoadRetirementTime(root.as_object()), is_random_spawn);

    double def_speed = LoadSpeed(root.as_object());
    size_t def_capacity = LoadBagCapacity(root.as_object());
    
    for(const auto& map : maps_in_game->as_array()) {
        game.AddMap(LoadMap(map.as_object(), loot_types, def_speed, def_capacity));
    }
               
    return game;
}

std::optional<player::JoiningInfo> LoadJoiningInfo(const string& req_post) {
    try {
        ValueJSON value = ParseJSON(req_post);

        string user_name = string(FindKey(value.as_object(), json_tag::USER_NAME)->as_string());  
        string map_id = string(FindKey(value.as_object(), json_tag::MAP_ID)->as_string());

        return player::JoiningInfo{user_name, Map::Id{map_id}};
    } catch(const std::exception&) {
        return std::nullopt;
    }
        
    ThrowUnidentifiedError(req_post);
}

std::optional<model::Direction> LoadUpdateInfo(const string& req_post) {
    try {
        ValueJSON value = ParseJSON(req_post);

        string dir_info = string(FindKey(value.as_object(), MOVE)->as_string());

        if(dir_info.empty()){
            return Direction::STOP;
        } else if(dir_info == UP) {
            return Direction::U;
        } else if(dir_info == DOWN) {
            return Direction::D;
        } else if(dir_info == LEFT) {
            return Direction::L;
        } else if(dir_info == RIGHT) {
            return Direction::R;
        }

        //Вернет nullopt, а значит ошибка загрузки запроса
        throw std::logic_error("Error parse request body");
    } catch(const std::exception&) {
        return std::nullopt;
    } 

    ThrowUnidentifiedError(req_post);
}

std::optional<int64_t> LoadTickInfo(const string& req_post) {
    try {
        ValueJSON value = ParseJSON(req_post);
        auto time = FindKey(value.as_object(), TIME_DELTA)->as_int64();
        return time;
    } catch(const std::exception&) {
        return std::nullopt;
    }
        
    ThrowUnidentifiedError(req_post);
}
} // namespace json_loader
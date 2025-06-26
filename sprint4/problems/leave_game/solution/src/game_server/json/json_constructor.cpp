#include <boost/asio/ip/tcp.hpp>

#include "json_constructor.h"

namespace json = boost::json;
namespace net = boost::asio;

using namespace json_tag;
using namespace model;
using namespace std::literals;
using namespace targets_storage;

using std::string;
using std::string_view;
using tcp = net::ip::tcp;

namespace {
    json::array MakeBodyRoads(const Map::Roads& roads) {
        json::array result;
        
        for(const auto& road : roads) {
            json::object road_obj;
            road_obj[X0] = road->GetStart().x;
            road_obj[Y0] = road->GetStart().y;

            if(road->IsHorizontal()) {
                road_obj[X1] = road->GetEnd().x;
            } else {
                road_obj[Y1] = road->GetEnd().y;
            }

            result.push_back(road_obj);
        }

        return result;
    }

    json::array MakeBodyBuildings(const Map::Buildings& buildings) {
        json::array result;
        
        for(const auto& building : buildings) {
            json::object buildig_obj;
            const auto bounds = building.GetBounds();
            buildig_obj[X] = bounds.position.x;
            buildig_obj[Y] = bounds.position.y;
            buildig_obj[W] = bounds.size.width;
            buildig_obj[H] = bounds.size.height;

            result.push_back(buildig_obj);
        }

        return result;
    }

    json::array MakeBodyOffices(const Map::Offices& offices) {
        json::array result;
        
        for(const auto& office : offices) {
            json::object office_obj;
            const auto office_pos = office.GetPosition();
            const auto office_offset = office.GetOffset();
            office_obj[ID] = *office.GetId();
            office_obj[X] = office_pos.x;
            office_obj[Y] = office_pos.y;
            office_obj[OFFSET_X] = office_offset.dx;
            office_obj[OFFSET_Y] = office_offset.dy;

            result.push_back(office_obj);
        }

        return result;
    }    

    json::value MakeBodyMapById(const Map& map, const json::array& types) {
        json::object result;
        result[ID] = *map.GetId();
        result[NAME] = map.GetName();
        result[ROADS] = MakeBodyRoads(map.GetRoads());
        result[BUILDINGS] = MakeBodyBuildings(map.GetBuildings());
        result[OFFICES] = MakeBodyOffices(map.GetOffices());
        result[LOOT_TYPES] = types;
        
        return result;
    }

    json::value MakeBodyMapsInfo(const Game::Maps& maps) {
        json::array result;

        for(const auto& map : maps) {
            json::object map_info;
            map_info[ID] = *map.GetId();
            map_info[NAME] = map.GetName();

            result.push_back(map_info);        
        }

        return result;
    }

    json::object MakeBodyStateDogs(const std::vector<std::shared_ptr<Dog>>& dogs) {
        json::object dogs_obj;

        for(const auto dog_it : dogs) {
            json::array position;
            json::array speed;
            json::array bag;
            json::object loot;
            json::object dog_info;
            

            CoordObject coord = dog_it->GetCoord();
            position.push_back(coord.x);
            position.push_back(coord.y);

            dog_info[POSITION] = position;

            SpeedUnit speed_unit = dog_it->GetSpeed();
            speed.push_back(speed_unit.horizontal);
            speed.push_back(speed_unit.vertical);

            dog_info[SPEED] = speed;
            dog_info[DIRECTION] = dog_it->GetDirectionToString();
            
            for(const auto& loot_it : dog_it->GetBag()) {
                loot[ID] = loot_it.GetId();
                loot[TYPE] = loot_it.GetType();
                bag.push_back(loot); 
            }
            dog_info[BAG] = bag;
            dog_info[SCORE] = dog_it->GetScore();
            
            dogs_obj[std::to_string(dog_it->GetId())] = dog_info;
        }

        return dogs_obj;
    }

    json::object MakeBodyStateLostObjects(const std::vector<model::Loot>& lost_objects) {
        json::object lost_obj;

        for(const auto& obj : lost_objects) {
            json::object obj_info;
            json::array pos_obj;

            obj_info[TYPE] = obj.GetType();

            CoordObject position = obj.GetPosition();
            pos_obj.push_back(position.x);
            pos_obj.push_back(position.y);

            obj_info[POSITION] = pos_obj;

            lost_obj[std::to_string(obj.GetId())] = obj_info; 
        }

        return lost_obj;
    }
}//namespace

namespace json_constructor {
//__________ResponseBody__________
string MakeBodyJSON(TargetRequestType req_type, 
                    const Game& game, 
                    const string& req_object,
                    const json::array* types) {
    switch (req_type) {
        case TargetRequestType::GET_MAPS_INFO :
            return json::serialize(MakeBodyMapsInfo(game.GetMaps())) + "\n";

        case TargetRequestType::GET_MAP_BY_ID : {
            Map::Id id{req_object};
            const auto map = game.FindMap(id);

            return map ? json::serialize(MakeBodyMapById(*map, *types)) + "\n"
                       : MakeBodyErrorJSON(TargetErrorCode::ERROR_SEARCH_MAP_CODE,
                                           TargetErrorMessage::ERROR_SEARCH_MAP_MESSAGE,
                                           req_object);
        }
        
        default:
            return MakeBodyErrorJSON(TargetErrorCode::ERROR_BAD_REQUEST_CODE,
                                     TargetErrorMessage::ERROR_BAD_REQUEST_MESSAGE,
                                     req_object);
    }
}

string MakeBodyJSON(const player::AuthorizationInfo& object) {
    json::object result;
            
    result[AUTH_TOKEN] = *object.token;
    result[PLAYER_ID] = object.player_id;

    return json::serialize(result) + "\n" ;
}

string MakeBodyJSON(const std::vector<std::shared_ptr<Dog>>& dogs) {
    json::object result;
    for(const auto dog_it : dogs) {
        json::object dog;  

        dog[NAME] = dog_it->GetName();
        result[std::to_string(dog_it->GetId())] = dog;
    }
 
    return json::serialize(result);
}

string MakeBodyJSON(const model::GameState& state) {
    json::object result;
    result[PLAYERS] = MakeBodyStateDogs(state.dogs);
    result[LOST_OBJECTS] = MakeBodyStateLostObjects(state.lost_objects);
    return json::serialize(result) + "\n";
} 

string MakeBodyJSON(const std::vector<player::PlayerRecord>& records) {
    json::array result;
    
    for(const auto& record : records) {
        json::object record_info;

        record_info[NAME] = record.name;
        record_info[SCORE] = record.score;

        double time_in_game  = static_cast<double>(record.total_time.count()) / MILLISECOND_PER_SECOND;     
        record_info[PLAY_TIME] = time_in_game;

        result.push_back(record_info);
    }

    return json::serialize(result);
}

string MakeBodyErrorJSON(string_view error_code,
                         string_view error_message, 
                         const string& req_object) {
    json::object result;
    result[CODE] = string(error_code);
    result[MESSAGE] = string(error_message) + req_object;

    return json::serialize(result) + "\n";
}

std::string MakeBodyEmptyObject() {
    return json::serialize(json::object()) + "\n";
}

//__________LogBody__________
json::object MakeLogStartJSON(int port, const string& address) {
    json::object val;

    val[PORT] = port;
    val[ADDRESS] = address;

    return val;
}

json::object MakeLogStopJSON(int code, const std::optional<string>& exception) {
    json::object val;

    val[CODE] = code;
    if (exception.has_value()) {
        val[EXEPTION] = exception.value();
    }

    return val;
}

json::object MakeLogRequestJSON(const string& ip, const string& uri, const string& method) {
    json::object val;

    val[IP] = ip;
    val[URI] = uri;
    val[METHOD] = method;

    return val;
}

json::object MakeLogResponceJSON(int response_time, int code, string_view content_type) {
    json::object val;

    val[RESPONSE_TIME] = response_time;
    val[CODE] = code;
    val[CONTENT_TYPE] = string(content_type);

    return val;
}

json::object MakeLogErrorJSON(int code, const string& text, const string& where) {
    json::object val;

    val[CODE] = code;
    val[TEXT] = text;
    val[WHERE] = where;

    return val;
}
} // namespace json_constructor
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
            road_obj[X0] = road.GetStart().x;
            road_obj[Y0] = road.GetStart().y;

            if(road.IsHorizontal()) {
                road_obj[X1] = road.GetEnd().x;
            } else {
                road_obj[Y1] = road.GetEnd().y;
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

    json::value MakeBodyMapById(const Map& map) {
        json::object result;
        result[ID] = *map.GetId();
        result[NAME] = map.GetName();
        result[ROADS] = MakeBodyRoads(map.GetRoads());
        result[BUILDINGS] = MakeBodyBuildings(map.GetBuildings());
        result[OFFICES] = MakeBodyOffices(map.GetOffices());

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

    json::value MakeBodyPlayers(const std::vector<Dog>& dogs) {
        json::object result;
        for(const auto dog_it : dogs) {
            json::object dog;  

            dog[NAME] = dog_it.GetName();
            result[std::to_string(dog_it.GetId())] = dog;
        }
 
        return result;
    }

    json::value MakeBodyStatePlayers(const std::vector<Dog>& dogs) {
        json::object result;
        json::object dogs_obj;

        for(const auto dog_it : dogs) {
            json::array position;
            json::array speed;
            json::object dog_info;

            CoordUnit coord = dog_it.GetCoord();
            position.push_back(coord.x);
            position.push_back(coord.y);

            dog_info[POSITION] = position;

            SpeedUnit speed_unit = dog_it.GetSpeed();
            speed.push_back(speed_unit.horizontal);
            speed.push_back(speed_unit.vertical);

            dog_info[SPEED] = speed;
            dog_info[DIRECTION] = dog_it.GetDirection();
            
            dogs_obj[std::to_string(dog_it.GetId())] = dog_info;
        }
        result[PLAYERS] = dogs_obj;

        return result;
    }
}//namespace

namespace json_constructor {
string MakeBodyJSON(TargetRequestType req_type, const Game& game, const string& req_object) {
    switch (req_type) {
        case TargetRequestType::GET_MAPS_INFO :
            return json::serialize(MakeBodyMapsInfo(game.GetMaps())) + "\n";
            break;

        case TargetRequestType::GET_MAP_BY_ID : {
            Map::Id id{req_object};
            const auto map = game.FindMap(id);

            return map ? json::serialize(MakeBodyMapById(*map)) + "\n"
                       : MakeBodyErrorJSON(TargetErrorCode::ERROR_SEARCH_MAP_CODE,
                                           TargetErrorMessage::ERROR_SEARCH_MAP_MESSAGE,
                                           req_object);
            break;
        }
        
        default:
            return MakeBodyErrorJSON(TargetErrorCode::ERROR_BAD_REQUEST_CODE,
                                     TargetErrorMessage::ERROR_BAD_REQUEST_MESSAGE,
                                     req_object);
            break;
    }
}

string MakeBodyJSON(TargetRequestType req_type, const player::AuthorizationInfo& object) {
    json::object result;

    switch (req_type) {
        case TargetRequestType::POST_JOIN_GAME : 
            
            result[AUTH_TOKEN] = *object.token;
            result[PLAYER_ID] = object.player_id;
            break;

        default:
            return MakeBodyErrorJSON(TargetErrorCode::ERROR_BAD_REQUEST_CODE,
                                     TargetErrorMessage::ERROR_BAD_REQUEST_MESSAGE);
            break;
    }

    return json::serialize(result) + "\n" ;
}

string MakeBodyJSON(TargetRequestType req_type, const std::vector<Dog>& dogs) {
     
    switch (req_type) {
        case TargetRequestType::GET_PLAYERS : {
            return json::serialize(MakeBodyPlayers(dogs)) + "\n";
            break;
        }            
        case TargetRequestType::GET_STATE : {
            return json::serialize(MakeBodyStatePlayers(dogs)) + "\n";
            break;
        }

        default:
        return MakeBodyErrorJSON(TargetErrorCode::ERROR_BAD_REQUEST_CODE,
                                 TargetErrorMessage::ERROR_BAD_REQUEST_MESSAGE);
            break;
    }
} 

string MakeBodyErrorJSON(string_view error_code,
                         string_view error_message, 
                         const string& req_object) {
    json::object result;
    result[CODE] = string(error_code);
    result[MESSAGE] = string(error_message) + req_object;

    return json::serialize(result) + "\n";
}

json::object MakeLogStartJSON(int port, const std::string& address) {
    boost::json::object val;

    val[PORT] = port;
    val[ADDRESS] = address;

    return val;
}

json::object MakeLogStopJSON(int code, const std::optional<std::string>& exception) {
    boost::json::object val;

    val[CODE] = code;
    if (exception.has_value()) {
        val[EXEPTION] = exception.value();
    }

    return val;
}

json::object MakeLogRequestJSON(const string& ip, const string& uri, const string& method) {
    boost::json::object val;

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

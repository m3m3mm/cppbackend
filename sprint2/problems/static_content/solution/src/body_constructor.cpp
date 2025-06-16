#include <boost/json.hpp>

#include <string_view>

#include "body_constructor.h"

namespace json = boost::json;

using namespace targets_storage;
using namespace model;
using namespace std::literals;

using std::string;
using std::string_view;

namespace {
    const string X = "x"s;
    const string Y = "y"s;
    const string W = "w"s;
    const string H = "h"s;
    const string X0 = "x0"s;
    const string X1 = "x1"s;
    const string Y0 = "y0"s;
    const string Y1 = "y1"s;
    const string OFFSET_X = "offsetX";
    const string OFFSET_Y = "offsetY";

    const string ID = "id"s;
    const string NAME = "name"s;

    const string ROADS = "roads"s;
    const string BUILDINGS = "buildings"s;
    const string OFFICES = "offices"s;

    const string CODE = "code"s;
    const string MESSAGE = "message"s;

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

    json::value MakeBodyError(string_view error_code,
                              string_view error_message, 
                              const string& req_object) {
        json::object result;
        result[CODE] = string(error_code);
        result[MESSAGE] = string(error_message) + req_object;

        return result;
    }
}//namespace

namespace body {
string MakeBodyJSON(targets_storage::TargetRequestType req_type, const Game& game, const string &req_object) {
    switch (req_type) {
        case TargetRequestType::GET_MAPS_INFO :
            return json::serialize(MakeBodyMapsInfo(game.GetMaps()));
            break;

        case TargetRequestType::GET_MAP_BY_ID : {
            Map::Id id{req_object};
            const auto map = game.FindMap(id);

            return map ? json::serialize(MakeBodyMapById(*map))
                       : json::serialize(MakeBodyError(TargetErrorCode::ERROR_SEARCH_MAP_CODE,
                                                       TargetErrorMessage::ERROR_SEARCH_MAP_MESSAGE,
                                                       req_object));
            break;
        }
        
        default:
            return json::serialize(MakeBodyError(TargetErrorCode::ERROR_BAD_REQUEST_CODE,
                                                 TargetErrorMessage::ERROR_BAD_REQUEST_MESSAGE,
                                                 req_object));
            break;
    }
}
} // namespace body
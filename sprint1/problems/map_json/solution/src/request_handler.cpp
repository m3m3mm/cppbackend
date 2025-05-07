#include "request_handler.h"
#include <boost/json.hpp>
#include <string>
#include <string_view>

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

http::response<http::string_body> RequestHandler::MakeErrorResponse(http::status status, beast::string_view code, beast::string_view message) {
    http::response<http::string_body> res{status, 11};
    res.set(http::field::content_type, "application/json");
    res.keep_alive(true);
    
    json::object error;
    error["code"] = code;
    error["message"] = message;
    
    res.body() = json::serialize(error);
    res.prepare_payload();
    return res;
}

http::response<http::string_body> RequestHandler::MakeSuccessResponse(http::status status, const json::value& body) {
    http::response<http::string_body> res{status, 11};
    res.set(http::field::content_type, "application/json");
    res.keep_alive(true);
    
    res.body() = json::serialize(body);
    res.prepare_payload();
    return res;
}

void RequestHandler::HandleGetMaps() {
    json::array maps;
    for (const auto& map : game_.GetMaps()) {
        json::object map_obj;
        map_obj["id"] = *map.GetId();
        map_obj["name"] = map.GetName();
        maps.push_back(std::move(map_obj));
    }
    
    send_(MakeSuccessResponse(http::status::ok, maps));
}

void RequestHandler::HandleGetMap(beast::string_view id) {
    if (auto map = game_.FindMap(model::Map::Id{std::string(id)})) {
        json::object map_obj;
        map_obj["id"] = *map->GetId();
        map_obj["name"] = map->GetName();
        
        // Add roads
        json::array roads;
        for (const auto& road : map->GetRoads()) {
            json::object road_obj;
            auto start = road.GetStart();
            auto end = road.GetEnd();
            road_obj["x0"] = start.x;
            road_obj["y0"] = start.y;
            if (road.IsHorizontal()) {
                road_obj["x1"] = end.x;
            } else {
                road_obj["y1"] = end.y;
            }
            roads.push_back(std::move(road_obj));
        }
        map_obj["roads"] = std::move(roads);
        
        // Add buildings
        json::array buildings;
        for (const auto& building : map->GetBuildings()) {
            json::object building_obj;
            auto bounds = building.GetBounds();
            building_obj["x"] = bounds.position.x;
            building_obj["y"] = bounds.position.y;
            building_obj["w"] = bounds.size.width;
            building_obj["h"] = bounds.size.height;
            buildings.push_back(std::move(building_obj));
        }
        map_obj["buildings"] = std::move(buildings);
        
        // Add offices
        json::array offices;
        for (const auto& office : map->GetOffices()) {
            json::object office_obj;
            office_obj["id"] = *office.GetId();
            auto pos = office.GetPosition();
            office_obj["x"] = pos.x;
            office_obj["y"] = pos.y;
            auto offset = office.GetOffset();
            office_obj["offsetX"] = offset.dx;
            office_obj["offsetY"] = offset.dy;
            offices.push_back(std::move(office_obj));
        }
        map_obj["offices"] = std::move(offices);
        
        send_(MakeSuccessResponse(http::status::ok, map_obj));
    } else {
        send_(MakeErrorResponse(http::status::not_found, "mapNotFound", "Map not found"));
    }
}

}  // namespace http_handler

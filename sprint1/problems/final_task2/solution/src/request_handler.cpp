// src/request_handler.cpp
#include "request_handler.h"
#include <boost/json.hpp>
#include <string>
#include <string_view>
#include <iostream> // Раскомментируйте для отладки, если потребуется
#include <iomanip>

namespace http_handler {

// Псевдонимы beast, http и json уже определены в заголовке через пространство имён http_handler

http::response<http::string_body> RequestHandler::MakeErrorResponse(
    http::status status, beast::string_view code, beast::string_view message,
    unsigned http_version, bool keep_alive) {
    http::response<http::string_body> res{status, http_version};
    res.set(http::field::content_type, "application/json");
    res.keep_alive(keep_alive); // Используем флаг keep_alive из запроса
    
    json::object error;
    error["code"] = code;
    error["message"] = message;
    
    res.body() = json::serialize(error);
    res.prepare_payload();
    return res;
}

http::response<http::string_body> RequestHandler::MakeSuccessResponse(
    http::status status, const json::value& body,
    unsigned http_version, bool keep_alive) {
    http::response<http::string_body> res{status, http_version};
    res.set(http::field::content_type, "application/json");
    res.keep_alive(keep_alive); // Используем флаг keep_alive из запроса
    
    res.body() = json::serialize(body);
    res.prepare_payload();
    return res;
}

// Метод теперь принимает константную ссылку на запрос и ссылку на колбэк
void RequestHandler::HandleGetMaps(const http::request<http::string_body>& req, StringResponseSendCallback& sender) {
    json::array maps_json_array;
    // Переименовал map в map_item, чтобы избежать возможного конфликта имен, если req содержит поле map
    for (const auto& map_item : game_.GetMaps()) { 
        json::object map_obj;
        map_obj["id"] = *map_item.GetId();
        map_obj["name"] = map_item.GetName();
        maps_json_array.push_back(std::move(map_obj));
    }
    
    // Используем версию HTTP и флаг keep_alive из переданного запроса
    sender(MakeSuccessResponse(http::status::ok, maps_json_array, req.version(), req.keep_alive()));
}

// Метод теперь принимает константную ссылку на запрос, ссылку на колбэк и ID карты
void RequestHandler::HandleGetMap(const http::request<http::string_body>& req, StringResponseSendCallback& sender, beast::string_view id_sv) {
    // Debug output
    std::cerr << "Requested map id: '" << id_sv << "' (length: " << id_sv.length() << ")\n";
    std::cerr << "Requested map id bytes: ";
    for (char c : id_sv) {
        std::cerr << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(c)) << " ";
    }
    std::cerr << std::dec << "\n";
    
    std::cerr << "Available map ids:\n";
    for (const auto& map : game_.GetMaps()) {
        const auto& id = *map.GetId();
        std::cerr << "  '" << id << "' (length: " << id.length() << ")\n";
        std::cerr << "  bytes: ";
        for (char c : id) {
            std::cerr << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(c)) << " ";
        }
        std::cerr << std::dec << "\n";
    }
    
    // Конструируем Map::Id из string_view
    model::Map::Id map_id_to_find{std::string(id_sv)};
    
    // Ищем карту по ID
    if (auto map_ptr = game_.FindMap(map_id_to_find)) {
        json::object map_obj;
        map_obj["id"] = *map_ptr->GetId();
        map_obj["name"] = map_ptr->GetName();
        
        json::array roads_json; // Переименовал, чтобы не конфликтовать с map_ptr->GetRoads()
        for (const auto& road : map_ptr->GetRoads()) {
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
            roads_json.push_back(std::move(road_obj));
        }
        map_obj["roads"] = std::move(roads_json);
        
        json::array buildings_json; // Переименовал
        for (const auto& building : map_ptr->GetBuildings()) {
            json::object building_obj;
            auto bounds = building.GetBounds();
            building_obj["x"] = bounds.position.x;
            building_obj["y"] = bounds.position.y;
            building_obj["w"] = bounds.size.width;
            building_obj["h"] = bounds.size.height;
            buildings_json.push_back(std::move(building_obj));
        }
        map_obj["buildings"] = std::move(buildings_json);
        
        json::array offices_json; // Переименовал
        for (const auto& office : map_ptr->GetOffices()) {
            json::object office_obj;
            office_obj["id"] = *office.GetId();
            auto pos = office.GetPosition();
            office_obj["x"] = pos.x;
            office_obj["y"] = pos.y;
            auto offset = office.GetOffset();
            office_obj["offsetX"] = offset.dx;
            office_obj["offsetY"] = offset.dy;
            offices_json.push_back(std::move(office_obj));
        }
        map_obj["offices"] = std::move(offices_json);
        
        sender(MakeSuccessResponse(http::status::ok, map_obj, req.version(), req.keep_alive()));
    } else {
        // Карта не найдена
        sender(MakeErrorResponse(http::status::not_found, "mapNotFound", "Map not found", req.version(), req.keep_alive()));
    }
}

}  // namespace http_handler

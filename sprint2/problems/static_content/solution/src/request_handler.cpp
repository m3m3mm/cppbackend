// src/request_handler.cpp
#include "request_handler.h"
#include <boost/json.hpp>
#include <string>
#include <string_view>
#include <iostream> // Раскомментируйте для отладки, если потребуется
#include <iomanip>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace http_handler {

// Псевдонимы beast, http и json уже определены в заголовке через пространство имён http_handler

http::response<http::string_body> RequestHandler::MakeErrorResponse(
    http::status status, beast::string_view code, beast::string_view message,
    unsigned http_version, bool keep_alive, bool plain_text) {
    http::response<http::string_body> res{status, http_version};
    
    if (plain_text) {
        res.set(http::field::content_type, "text/plain");
        res.body() = std::string(message);
    } else {
        res.set(http::field::content_type, "application/json");
        json::object error;
        error["code"] = code;
        error["message"] = message;
        res.body() = json::serialize(error);
    }
    
    res.keep_alive(keep_alive);
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

json::object SerializeRoad(const model::Road& road) {
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
    return road_obj;
}

json::object SerializeBuilding(const model::Building& building) {
    json::object building_obj;
    auto bounds = building.GetBounds();
    building_obj["x"] = bounds.position.x;
    building_obj["y"] = bounds.position.y;
    building_obj["w"] = bounds.size.width;
    building_obj["h"] = bounds.size.height;
    return building_obj;
}

json::object SerializeOffice(const model::Office& office) {
    json::object office_obj;
    office_obj["id"] = *office.GetId();
    auto pos = office.GetPosition();
    office_obj["x"] = pos.x;
    office_obj["y"] = pos.y;
    auto offset = office.GetOffset();
    office_obj["offsetX"] = offset.dx;
    office_obj["offsetY"] = offset.dy;
    return office_obj;
}

json::object SerializeMap(const model::Map& map) {
    json::object map_obj;
    map_obj["id"] = *map.GetId();
    map_obj["name"] = map.GetName();
    
    json::array roads_json;
    for (const auto& road : map.GetRoads()) {
        roads_json.push_back(SerializeRoad(road));
    }
    map_obj["roads"] = std::move(roads_json);
    
    json::array buildings_json;
    for (const auto& building : map.GetBuildings()) {
        buildings_json.push_back(SerializeBuilding(building));
    }
    map_obj["buildings"] = std::move(buildings_json);
    
    json::array offices_json;
    for (const auto& office : map.GetOffices()) {
        offices_json.push_back(SerializeOffice(office));
    }
    map_obj["offices"] = std::move(offices_json);
    
    return map_obj;
}

// Метод теперь принимает константную ссылку на запрос и ссылку на колбэк
void RequestHandler::HandleGetMaps(const http::request<http::string_body>& req, StringResponseSendCallback& sender) {
    json::array maps_json_array;
    for (const auto& map_item : game_.GetMaps()) {
        json::object map_obj;
        map_obj["id"] = *map_item.GetId();
        map_obj["name"] = map_item.GetName();
        maps_json_array.push_back(std::move(map_obj));
    }
    
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
    
    model::Map::Id map_id_to_find{std::string(id_sv)};
    
    if (auto map_ptr = game_.FindMap(map_id_to_find)) {
        sender(MakeSuccessResponse(http::status::ok, SerializeMap(*map_ptr), req.version(), req.keep_alive()));
    } else {
        sender(MakeErrorResponse(http::status::not_found, "mapNotFound", "Map not found", req.version(), req.keep_alive(), false));
    }
}

void RequestHandler::HandleStaticFile(const http::request<http::string_body>& req, StringResponseSendCallback& sender, bool head_only) {
    const auto& target = req.target();
    unsigned http_version = req.version();
    bool keep_alive = req.keep_alive();

    // URL decode the path
    std::string decoded_path = UrlDecode(std::string(target));
    
    // Handle root path and index.html
    if (decoded_path == "/" || decoded_path == "/index.html") {
        decoded_path = "/index.html";
    }

    // Check for path traversal attempts
    if (IsPathTraversal(decoded_path)) {
        sender(MakeErrorResponse(http::status::not_found, "notFound", "File not found", http_version, keep_alive, true));
        return;
    }

    // Construct the full file path
    fs::path file_path = fs::path(static_files_dir_) / decoded_path.substr(1);

    // Check if file exists
    if (!fs::exists(file_path)) {
        sender(MakeErrorResponse(http::status::not_found, "notFound", "File not found", http_version, keep_alive, true));
        return;
    }

    // Check if it's a directory
    if (fs::is_directory(file_path)) {
        file_path /= "index.html";
        if (!fs::exists(file_path)) {
            sender(MakeErrorResponse(http::status::not_found, "notFound", "Directory index not found", http_version, keep_alive, true));
            return;
        }
    }

    // Get file size
    auto file_size = fs::file_size(file_path);

    // Create response
    http::response<http::string_body> res{http::status::ok, http_version};
    res.set(http::field::content_type, GetMimeType(file_path.string()));
    res.set(http::field::content_length, std::to_string(file_size));
    res.keep_alive(keep_alive);

    // For HEAD requests, we don't need to read the file
    if (!head_only) {
        // Read file content
        std::ifstream file(file_path, std::ios::binary);
        if (!file) {
            sender(MakeErrorResponse(http::status::internal_server_error, "internalError", "Failed to read file", http_version, keep_alive, true));
            return;
        }

        std::string content;
        content.resize(file_size);
        file.read(content.data(), file_size);
        res.body() = std::move(content);
    }

    sender(std::move(res));
}

std::string RequestHandler::GetMimeType(const std::string& path) {
    auto ext = fs::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    auto it = mime_types_.find(ext);
    if (it != mime_types_.end()) {
        return it->second;
    }
    return "application/octet-stream";
}

std::string RequestHandler::UrlDecode(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            int value;
            std::istringstream iss(str.substr(i + 1, 2));
            iss >> std::hex >> value;
            if (!iss.fail()) {
                result += static_cast<char>(value);
                i += 2;
                continue;
            }
        }
        result += str[i];
    }
    
    return result;
}

bool RequestHandler::IsPathTraversal(const std::string& path) {
    // Check for ".." in the path
    if (path.find("..") != std::string::npos) {
        return true;
    }
    
    // Check if the normalized path starts with the static files directory
    fs::path normalized_path = fs::path(static_files_dir_) / path.substr(1);
    fs::path static_dir = fs::path(static_files_dir_);
    
    try {
        normalized_path = fs::canonical(normalized_path);
        static_dir = fs::canonical(static_dir);
        
        // Check if the normalized path is outside the static directory
        return normalized_path.string().find(static_dir.string()) != 0;
    } catch (const fs::filesystem_error&) {
        return true;
    }
}

}  // namespace http_handler

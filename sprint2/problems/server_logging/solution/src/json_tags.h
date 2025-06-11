#pragma once

#include <string>

namespace json_tag {
    //identification tags
    const static std::string ID = "id";
    const static std::string NAME = "name";

    //coordinate tags
    const static std::string X = "x";
    const static std::string Y = "y";
    const static std::string W = "w";
    const static std::string H = "h";
    const static std::string X0 = "x0";
    const static std::string X1 = "x1";
    const static std::string Y0 = "y0";
    const static std::string Y1 = "y1";
    const static std::string OFFSET_X = "offsetX";
    const static std::string OFFSET_Y = "offsetY";

    //infrastructure tags
    const static std::string ROADS = "roads";
    const static std::string BUILDINGS = "buildings";
    const static std::string OFFICES = "offices";
    const static std::string MAPS = "maps";

    //log tags
    const static std::string PORT = "port";
    const static std::string ADDRESS = "address";
    const static std::string EXEPTION = "exeption";
    const static std::string IP = "ip";
    const static std::string URI = "URI";
    const static std::string METHOD = "method";
    const static std::string RESPONSE_TIME = "response_time";
    const static std::string CONTENT_TYPE = "content_type";
    const static std::string TEXT = "text";
    const static std::string WHERE = "where";

    //report tags
    const static std::string ERROR_COORD_NOT_FOUND = "x1 or y1 not found";
    const static std::string ERROR_KEY_NOT_FOUND = "Not found key: ";
    const static std::string ERROR_OPENING_FILE = "Error opening file: ";
    const static std::string ERROR_LOADING_ROAD = "Error loading road: ";
    const static std::string ERROR_LOADING_OFFICE  = "Error loading office: ";
    const static std::string ERROR_LOADING_BUILDING ="Error loading building: ";
    const static std::string UNIDENTIFIED_ERROR = "Unidentified error";

    //other tags
    const static std::string CODE = "code";
    const static std::string MESSAGE = "message";
}//namespace json_tag
#pragma once

#include <string>

namespace json_tag {
    //identification tags    
     const static std::string ID = "id";
     const static std::string MAP_ID = "mapId";
     const static std::string PLAYER_ID = "playerId";
     const static std::string NAME = "name";
     const static std::string USER_NAME = "userName";
     const static std::string AUTH_TOKEN = "authToken";
     const static std::string PLAYERS = "players";

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

    //direction tags
    const static std::string UP = "U";
    const static std::string DOWN = "D";
    const static std::string LEFT = "L";
    const static std::string RIGHT = "R";

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

    //loot tags
    const static std::string LOOT_GENERATOR_CONFIG = "lootGeneratorConfig";
    const static std::string PERIOD = "period";
    const static std::string PROBABILITY = "probability";
    const static std::string LOOT_TYPES = "lootTypes";
    const static std::string LOST_OBJECTS = "lostObjects";
    const static std::string TYPE = "type";
    const static std::string VALUE = "value";

    //report tags
    const static std::string ERROR_COORD_NOT_FOUND = "x1 or y1 not found";
    const static std::string ERROR_KEY_NOT_FOUND = "Not found key: ";
    const static std::string ERROR_OPENING_FILE = "Error opening file: ";
    const static std::string ERROR_LOADING_ROAD = "Error loading road: ";
    const static std::string ERROR_LOADING_OFFICE  = "Error loading office: ";
    const static std::string ERROR_LOADING_BUILDING ="Error loading building: ";
    const static std::string ERROR_EMPTY_LOOT_TYPES = "Empty loot types array";
    const static std::string UNIDENTIFIED_ERROR = "Unidentified error";

    //unit tags
    const static std::string DIRECTION = "dir";
    const static std::string SPEED = "speed";
    const static std::string POSITION = "pos";
    const static std::string DEFAULT_DOG_SPEED = "defaultDogSpeed";
    const static std::string DOG_SPEED = "dogSpeed";
    const static std::string DEFAULT_BAG_CAPACITY = "defaultBagCapacity";
    const static std::string BAG_CAPACITY = "bagCapacity";
    const static std::string BAG = "bag";
    const static std::string SCORE = "score";
    const static std::string DOG_RETIREMENT_TIME = "dogRetirementTime";
    const static std::string PLAY_TIME = "playTime";

    //other tags
    const static std::string CODE = "code";
    const static std::string MESSAGE = "message";
    const static std::string TIME_DELTA = "timeDelta";
    const static std::string MOVE = "move";
}//namespace json_tag
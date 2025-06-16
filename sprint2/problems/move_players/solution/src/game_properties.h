#pragma once

#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "model.h"

namespace model {
class GameSession {
public:
    GameSession(const Map& map) : map_(map) {
    }

    std::shared_ptr<Dog> AddDog(const std::string& name);

    Map::Id GetMapId() const;
    const Map& GetMap() const;
    const std::vector<Dog>& GetDogs() const;
private:
    std::vector<Dog> dogs_;
    const Map& map_;
    size_t next_id = 0;

    const Road& GenerateRandomRoad();
    CoordUnit GenerateRandomPosition();
    CoordUnit GetDefPosition();    
};

struct UnitParameters {
    std::shared_ptr<GameSession> session;
    std::shared_ptr<Dog> dog;
};

class Game {
public:
    using Maps = std::vector<Map>;

    void AddMap(Map map);
    void AddSpeed(double speed);

    UnitParameters PrepareUnitParameters(const Map::Id& map_id,const std::string& name);

    const Map* FindMap(const Map::Id& id) const noexcept;
    Map* FindMap(const Map::Id& id);

    const Maps& GetMaps() const noexcept;
    const double GetSpeed() const noexcept;  
private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToSession = std::unordered_map<Map::Id, std::shared_ptr<GameSession>, MapIdHasher>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::vector<GameSession> sessions_;
    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;
    MapIdToSession map_id_to_session_;
    
    double def_speed = 0.;
    size_t next_id = 0;   

    std::shared_ptr<GameSession> FindGameSessionById(const model::Map::Id& map_id) const;
};

}//namespace model
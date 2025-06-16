#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "model.h"

namespace model {
class GameSession {
public:
    GameSession(const Map& map);

    std::shared_ptr<Dog> AddDog(const std::string& name, bool is_random);

    void MoveUnits(const std::chrono::milliseconds& time_delta) const;

    Map::Id GetMapId() const;
    const Map& GetMap() const;
    const std::vector<std::shared_ptr<Dog>>& GetDogs() const;
private:
    std::vector<std::shared_ptr<Dog>> dogs_;
    const Map& map_;
    size_t next_id = 0;

    double ComputeDistance(CoordUnit lhs, CoordUnit rhs) const;
    std::optional<CoordUnit> ComputeAllowedPosition(CoordUnit cur_pos, CoordUnit new_pos) const;
     
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

    Game() = default;
    Game(bool is_random);

    void AddMap(Map map);
    void AddSpeed(double speed);

    UnitParameters PrepareUnitParameters(const Map::Id& map_id,const std::string& name);

    const Map* FindMap(const Map::Id& id) const noexcept;
    Map* FindMap(const Map::Id& id);

    const Maps& GetMaps() const noexcept;
    const double GetSpeed() const noexcept;
    const std::vector<std::shared_ptr<GameSession>>& GetSessions() const;
      
private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToSession = std::unordered_map<Map::Id, std::shared_ptr<GameSession>, MapIdHasher>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::vector<std::shared_ptr<GameSession>> sessions_;
    std::vector<Map> maps_;
    
    MapIdToIndex map_id_to_index_;
    MapIdToSession map_id_to_session_;
    
    double def_speed = 0.;
    size_t next_id = 0;   
    bool randomize_spawn_ = false;

    std::shared_ptr<GameSession> FindGameSessionById(const model::Map::Id& map_id) const;
};

}//namespace model
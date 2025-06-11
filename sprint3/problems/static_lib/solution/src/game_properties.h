#pragma once

#include <chrono>
#include <cmath>
#include <list>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "model.h"
#include "loot_generator.h"

namespace model {
struct LootGeneratorConfig {
    double period  = 0.;
    double probability = 0.; 
};

class Loot {
public:
    Loot(size_t id, size_t type, CoordObject position);

    size_t GetId() const;
    size_t GetType() const;
    CoordObject GetPosition() const;
private:
    size_t id_;
    size_t type_;
    CoordObject position_;
    
};

class GameSession {
public:
    GameSession(const Map& map, loot_gen::LootGenerator&& loot_generator);

    std::shared_ptr<Dog> AddDog(const std::string& name, bool is_random);

    void MoveUnits(const std::chrono::milliseconds& delta) const;
    void GenerateLoot(const std::chrono::milliseconds& delta);

    Map::Id GetMapId() const;
    const Map& GetMap() const;
    const std::vector<std::shared_ptr<Dog>>& GetDogs() const;
    const std::list<Loot>& GetLoot() const;
private:
    struct PositionHasher{
        size_t operator()(CoordObject pos) const;
        std::hash<double> d_hash;
    };

    loot_gen::LootGenerator loot_generator_;
    std::vector<std::shared_ptr<Dog>> dogs_;

    std::list<Loot> lost_objects_;
    std::unordered_map<CoordObject, const Loot*, PositionHasher> object_to_pos_;

    const Map& map_;
    size_t next_dog_id = 0;
    size_t next_loot_id = 0;

    double ComputeDistance(CoordObject lhs, CoordObject rhs) const;
    std::optional<CoordObject> ComputeAllowedPosition(CoordObject cur_pos, CoordObject new_pos) const;
    
    size_t GenerateRandomLootType();
    const Road& GenerateRandomRoad();
    CoordObject GenerateRandomPosition();
    CoordObject GetDefPosition();  
};

struct UnitParameters {
    std::shared_ptr<GameSession> session;
    std::shared_ptr<Dog> dog;
};

class Game {
public:
    using Maps = std::vector<Map>;

    Game() = default;
    Game(bool is_random, LootGeneratorConfig loot_gen_conf);

    void AddMap(Map map);
    void AddSpeed(double speed);

    UnitParameters PrepareUnitParameters(const Map::Id& map_id,const std::string& name);
    void ProcessTickActions(const std::chrono::milliseconds& delta) const;

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
    
    LootGeneratorConfig loot_generator_config_;
    double def_speed = 0.;
    size_t next_id = 0;   
    bool randomize_spawn_ = false;

    std::shared_ptr<GameSession> FindGameSessionById(const model::Map::Id& map_id) const;
};

}//namespace model
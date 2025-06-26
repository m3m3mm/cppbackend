#pragma once

#include <chrono>
#include <cmath>
#include <list>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <set>
#include <vector>

#include "detail/collision_detector.h"
#include "detail/loot_generator.h"
#include "dynamic_object_properties.h"
#include "static_object_prorerties.h"

namespace model {
const static int64_t RETIREMENT_TIME = 60000;
const static double MILLISECOND_PER_SECOND = 1000.;

struct LootGeneratorConfig {
    double period  = 0.;
    double probability = 0.; 
};

struct GameState {
    const std::vector<std::shared_ptr<model::Dog>>& dogs;
    const std::vector<model::Loot>& lost_objects;
};

class GameSession {
public:
    using Dogs = std::vector<std::shared_ptr<Dog>>;
    using LostObjects = std::vector<Loot>;

    GameSession(loot_gen::LootGenerator&& loot_generator, const Map& map);

    DogPtr AddDog(const std::string& name, bool is_random);
    void AddDogs(Dogs&& dogs);
    void AddLostObjects(LostObjects&& lost_objects);

    void MoveUnits(const std::chrono::milliseconds& delta);
    void GenerateLoot(const std::chrono::milliseconds& delta);

    DogPtr FindDog(size_t id);

    Map::Id GetMapId() const;
    const Map& GetMap() const;
    const std::vector<DogPtr>& GetDogs() const;
    const std::vector<Loot>& GetLoot() const;

    void DeleteDog(size_t dog_id);
private:
    loot_gen::LootGenerator loot_generator_;
    std::vector<DogPtr> dogs_;
    std::vector<Loot> lost_objects_;

    const Map& map_;
    size_t next_dog_id_ = 0;
    size_t next_loot_id_ = 0;

    double ComputeDistance(CoordObject lhs, CoordObject rhs) const;
    std::optional<CoordObject> ComputeAllowedPosition(CoordObject cur_pos, CoordObject new_pos) const;
    void ProcessLoot();
    
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
    using GameSessions = std::vector<std::shared_ptr<GameSession>>;

    Game() = default;
    Game(LootGeneratorConfig loot_gen_conf, int64_t retirement_time, bool is_random);

    void AddMap(Map map);
    std::shared_ptr<GameSession> AddSession(const Map::Id& map_id);

    UnitParameters PrepareUnitParameters(const Map::Id& map_id, const std::string& name);
    UnitParameters PrepareUnitParameters(const Map::Id& map_id, size_t dog_id);
    void ProcessTickActions(const std::chrono::milliseconds& delta) const;

    const Map* FindMap(const Map::Id& id) const noexcept;
    Map* FindMap(const Map::Id& id);

    const Maps& GetMaps() const noexcept;
    const GameSessions& GetSessions() const;
    const LootGeneratorConfig GetGeneratorConfig() const;
    std::chrono::milliseconds GetRetirementTime() const;
private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToSession = std::unordered_map<Map::Id, std::shared_ptr<GameSession>, MapIdHasher>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    GameSessions sessions_;
    std::vector<Map> maps_;
    
    MapIdToIndex map_id_to_index_;
    MapIdToSession map_id_to_session_;
    
    LootGeneratorConfig loot_generator_config_; 
    
    std::chrono::milliseconds retirement_time_;
    bool randomize_spawn_ = false;

    std::shared_ptr<GameSession> FindGameSessionById(const model::Map::Id& map_id) const;
};

}//namespace model
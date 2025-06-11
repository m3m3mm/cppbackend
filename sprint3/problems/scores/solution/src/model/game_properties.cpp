#include <stdexcept>

#include "detail/geom.h"
#include "game_properties.h"

namespace model {
using namespace collision_detector;
using namespace std::literals;
using namespace loot_gen;

using std::shared_ptr;
using std::string;

const static SpeedUnit DEF_SPEED{0.0,0.0};
const static double MILLISECOND_PER_SECOND = 1000.;

const static double LOST_OBJECT_WIDTH = 0.0;
const static double DOG_WIDTH = 0.6;
const static double OFFICE_WIDTH = 0.6;

//__________GameSession__________
GameSession::GameSession(const Map& map, LootGenerator&& loot_generator) 
    : loot_generator_(std::forward<LootGenerator>(loot_generator))
    , map_(map){
}

shared_ptr<Dog> GameSession::AddDog(const string& name, bool is_random) {  
    dogs_.emplace_back(std::make_shared<Dog>(name, 
                                             next_dog_id++, 
                                             map_.GetBagCapacity(),
                                             is_random ? GenerateRandomPosition()
                                                       : GetDefPosition()));                                                     
    return dogs_.back();
}

void GameSession::MoveUnits(const std::chrono::milliseconds& delta) {
    for(const auto dog : dogs_) {
        const auto position = dog->GetCoord();
        const auto speed = dog->GetSpeed();

        CoordObject reqested_position = {position.x + speed.horizontal * delta.count() / MILLISECOND_PER_SECOND,
                                         position.y + speed.vertical * delta.count() / MILLISECOND_PER_SECOND};

        auto allowed_position = ComputeAllowedPosition(position, reqested_position);

        if(allowed_position) {
            dog->Move(*allowed_position);
            
            if(allowed_position->x != reqested_position.x
               || allowed_position->y != reqested_position.y) {
                dog->UpdateState(DEF_SPEED, dog->GetDirection());
            }
        }

        ProcessLoot();
    }
}

void GameSession::GenerateLoot(const std::chrono::milliseconds& delta) {
    size_t new_objects_count = loot_generator_.Generate(delta, lost_objects_.size(), dogs_.size());

    for(size_t i = 0; i < new_objects_count; ++i) {
        size_t type = GenerateRandomLootType();
        size_t cost = map_.GetTypeCost(type);
        lost_objects_.emplace_back(next_loot_id++, type, cost, GenerateRandomPosition());
    }
}

Map::Id GameSession::GetMapId() const {
    return map_.GetId();
}

const Map& GameSession::GetMap() const {
    return map_;
}

const std::vector<std::shared_ptr<Dog>>& GameSession::GetDogs() const {
    return dogs_;
}

const std::vector<Loot>& GameSession::GetLoot() const {
    return lost_objects_;
}

double GameSession::ComputeDistance(CoordObject lhs, CoordObject rhs) const {
    double distance = std::pow(rhs.x - lhs.x, 2) + std::pow(rhs.y - lhs.y, 2);

    return std::sqrt(distance);
}

std::optional<CoordObject> GameSession::ComputeAllowedPosition(CoordObject cur_pos, CoordObject new_pos) const {
    const auto roads = map_.FindCandidateRoads(cur_pos);

    if(roads.empty()) {
        return std::nullopt;
    }

    CoordObject max_allowed_pos = cur_pos;
    double max_distance = ComputeDistance(cur_pos, cur_pos);

    for (const auto& road : roads) {
        if(road->IsCurrectPos(new_pos)) {
            return new_pos;
        }

        CoordObject allowed_pos = road->Clamp(new_pos);
        double distance = ComputeDistance(cur_pos, allowed_pos);

        if (distance > max_distance) {
            max_allowed_pos = allowed_pos;
            max_distance = distance;
        }
    }

    return max_allowed_pos;
}

void GameSession::ProcessLoot() {
    const auto& offices = map_.GetOffices();

    ItemGathererProvider::Items items;
    items.reserve(lost_objects_.size() + offices.size());

    for (const auto& obj : lost_objects_) {
        auto pos = obj.GetPosition();
        items.emplace_back(geom::Point2D{pos.x, pos.y}, LOST_OBJECT_WIDTH/2.);
    }

    size_t offices_start = items.size();
    for (const auto& office : offices) {
        auto pos = office.GetPosition();
        geom::Point2D point{static_cast<double>(pos.x), static_cast<double>(pos.y)};
        items.emplace_back(point, OFFICE_WIDTH/2.);
    }

    ItemGathererProvider::Gatherers gatherers;
    gatherers.reserve(dogs_.size());

    for (const auto& dog : dogs_) {
        auto prev_pos = dog->GetPrevCoord();
        auto new_pos = dog->GetCoord();
        geom::Point2D prev_pos_2d{prev_pos.x, prev_pos.y};
        geom::Point2D new_pos_2d{new_pos.x, new_pos.y};
        gatherers.emplace_back(prev_pos_2d, new_pos_2d, DOG_WIDTH/2.);
    }

    auto events = FindGatherEvents(ItemGathererProvider(std::move(items), std::move(gatherers)));

    if (events.empty()) {
        return;
    }

    for (const auto& event : events) {
        auto& dog = dogs_[event.gatherer_id];
        auto& bag = dog->GetBag();

        if(event.item_id < offices_start) {
            auto& obj = lost_objects_[event.item_id];
            if(!obj.IsPickedUp()) {
                dog->TryPickUpLoot(obj);
            }
        } else {
            if(!bag.empty()) {
                dog->LayOutLoot();
            }
        }
    }

    std::erase_if(lost_objects_, [](const auto& obj) {
        return obj.IsPickedUp();
    });
}

size_t GameSession::GenerateRandomLootType() {
    std::random_device random_device_;
    std::mt19937_64 generator(random_device_());
    std::uniform_int_distribution<> dist(0, map_.GetLootTypesCount() - 1);

    return dist(generator);
}

const Road &GameSession::GenerateRandomRoad() {
    std::random_device random_device_;
    std::mt19937_64 generator(random_device_());
    std::uniform_int_distribution<> dist(0, map_.GetRoads().size() - 1);

    return *map_.GetRoads()[dist(generator)];
}

CoordObject GameSession::GenerateRandomPosition() {
    std::random_device random_device_;
    auto road = GenerateRandomRoad();

    std::mt19937_64 generator(random_device_());
    auto bounds = road.GetBounds();


    std::uniform_real_distribution<> dist_start_end(bounds.lower, bounds.upper);
    std::uniform_real_distribution<> dist_left_right(bounds.left, bounds.right);
    
    double gen_start_end = (dist_start_end(generator));
    double gen_left_right = (dist_left_right(generator));
    
    return road.IsHorizontal() ? CoordObject{gen_start_end, gen_left_right}
                               : CoordObject{gen_left_right, gen_start_end};
}

CoordObject GameSession::GetDefPosition() {
    auto road = map_.GetRoads()[0];
    double start_x = static_cast<double>(road->GetStart().x);
    double start_y = static_cast<double>(road->GetStart().y);
    
    return {start_x, start_y};
}

//__________Game__________
Game::Game(bool is_random, LootGeneratorConfig loot_gen_conf) 
    : randomize_spawn_(is_random)
    , loot_generator_config_(loot_gen_conf) {
}

void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

UnitParameters Game::PrepareUnitParameters(const Map::Id& map_id, const string& name) {
    auto game_session = FindGameSessionById(map_id);

    if(!game_session) {
        int64_t period = static_cast<int64_t>(loot_generator_config_.period * MILLISECOND_PER_SECOND);
        game_session = (sessions_.emplace_back(std::make_shared<GameSession>(*FindMap(map_id),
                                                                             LootGenerator(std::chrono::milliseconds(period), 
                                                                                           loot_generator_config_.probability))));
        map_id_to_session_.insert({map_id, game_session});
    }

    auto dog = FindGameSessionById(map_id)->AddDog(name, randomize_spawn_);
    return UnitParameters{game_session, dog};
}

void Game::ProcessTickActions(const std::chrono::milliseconds& delta) const {
    for(const auto& session : GetSessions()) {
        session->MoveUnits(delta);
        session->GenerateLoot(delta);
    }
}

const Map* Game::FindMap(const Map::Id& id) const noexcept {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

Map* Game::FindMap(const Map::Id& id) {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

const Game::Maps& Game::GetMaps() const noexcept {
    return maps_;
}

const std::vector<std::shared_ptr<GameSession>>& Game::GetSessions() const {
    return sessions_;
}

shared_ptr<GameSession> Game::FindGameSessionById(const model::Map::Id& map_id) const {
    if (auto it = map_id_to_session_.find(map_id); it != map_id_to_session_.end()) {
        return it->second;
    }

    return nullptr;
}
} // namespace model
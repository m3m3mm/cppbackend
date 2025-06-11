#include <stdexcept>

#include "game_properties.h"

namespace model {
using namespace std::literals;
using namespace loot_gen;

using std::shared_ptr;
using std::string;

const static SpeedUnit DEF_SPEED{0.0,0.0};
const static double MILLISECOND_PER_SECOND = 1000.;

//__________Loot__________
Loot::Loot(size_t id, size_t type, CoordObject position)
    : id_(id)
    , type_(type)
    , position_(position) {
}

size_t Loot::GetId() const {
    return id_;
}

size_t Loot::GetType() const {
    return type_;
}

CoordObject Loot::GetPosition() const {
    return position_;
}

//__________GameSession__________
GameSession::GameSession(const Map& map, LootGenerator&& loot_generator) 
    : loot_generator_(std::forward<LootGenerator>(loot_generator))
    , map_(map){
}

shared_ptr<Dog> GameSession::AddDog(const string& name, bool is_random) {  
    dogs_.emplace_back(std::make_shared<Dog>(name, next_dog_id++, is_random ? GenerateRandomPosition()
                                                                        : GetDefPosition()));     
    return dogs_.back();
}

void GameSession::MoveUnits(const std::chrono::milliseconds& delta) const {
    for(const auto dog : dogs_) {
        const auto& position = dog->GetCoord();
        const auto& speed = dog->GetSpeed();

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
    }
}

void GameSession::GenerateLoot(const std::chrono::milliseconds& delta) {
    size_t new_objects_count = loot_generator_.Generate(delta, lost_objects_.size(), dogs_.size());

    for(size_t i = 0; i < new_objects_count; ++i) {
        auto& ptr_loot = lost_objects_.emplace_back(next_loot_id++, GenerateRandomLootType(), GenerateRandomPosition());
        object_to_pos_.insert({ptr_loot.GetPosition(), &ptr_loot});
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

const std::list<Loot>& GameSession::GetLoot() const {
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

    double start_road = 0.;
    double end_road = 0.;
    double position_road = 0.;

    std::mt19937_64 generator(random_device_());
    bool IsHorizontal = road.IsHorizontal();

    if(IsHorizontal) {
        start_road = static_cast<double>(road.GetStart().x);
        end_road = static_cast<double>(road.GetEnd().x);
        position_road = static_cast<double>(road.GetStart().y);
    } else {
        start_road = static_cast<double>(road.GetStart().y);
        end_road = static_cast<double>(road.GetEnd().y);
        position_road = static_cast<double>(road.GetStart().x);
    }

    std::uniform_real_distribution<> dist(start_road, end_road);
    
    double gen_coord = (std::round(dist(generator) * 10) / 10);
    
    return IsHorizontal ? CoordObject{gen_coord, position_road}
                        : CoordObject{position_road, gen_coord};
}

CoordObject GameSession::GetDefPosition() {
    auto road = map_.GetRoads()[0];
    double start_x = static_cast<double>(road->GetStart().x);
    double start_y = static_cast<double>(road->GetStart().y);
    
    return {start_x, start_y};
}

size_t GameSession::PositionHasher::operator()(CoordObject pos) const {
    size_t d_x = d_hash(pos.x);
    size_t d_y = d_hash(pos.y);
     
    return std::pow(d_x, 2) + std::pow(d_y, 3);
};

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

void Game::AddSpeed(double speed) {
    def_speed = speed;
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

const double Game::GetSpeed() const noexcept {
    return def_speed;
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
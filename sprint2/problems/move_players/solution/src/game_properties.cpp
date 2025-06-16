#include <stdexcept>

#include "game_properties.h"

namespace model {
using namespace std::literals;

using std::shared_ptr;
using std::string;

//__________GameSession__________
shared_ptr<Dog> GameSession::AddDog(const string& name) {  
    dogs_.push_back(Dog(name, next_id++, GenerateRandomPosition()));     
    return std::make_shared<Dog>(dogs_.back());
}

Map::Id GameSession::GetMapId() const {
    return map_.GetId();
}

const Map& GameSession::GetMap() const {
    return map_;
}

const std::vector<Dog>& GameSession::GetDogs() const {
    return dogs_;
}

const Road& GameSession::GenerateRandomRoad() {
    std::random_device random_device_;
    std::mt19937_64 generator(random_device_());
    std::uniform_int_distribution<> dist(0, map_.GetRoads().size() - 1);

    return map_.GetRoads()[dist(generator)];
}

CoordUnit GameSession::GenerateRandomPosition() {
    std::random_device random_device_;
    auto road = GenerateRandomRoad();

    double start_road = 0;
    double end_road = 0;
    double position_road = 0;

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
    return IsHorizontal ? CoordUnit{dist(generator), position_road}
                        : CoordUnit{position_road, dist(generator)};
}

CoordUnit GameSession::GetDefPosition() {
    auto road = map_.GetRoads()[0];
    double start_x = static_cast<double>(road.GetStart().x);
    double start_y = static_cast<double>(road.GetStart().y);
    
    return {start_x, start_y};
}

//__________Game__________
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

UnitParameters Game::PrepareUnitParameters(const Map::Id& map_id, const std::string& name) {
    auto game_session = FindGameSessionById(map_id);

    if(!game_session) {
        game_session = std::make_shared<GameSession>(sessions_.emplace_back(*FindMap(map_id)));
        map_id_to_session_.insert({map_id, game_session});
    }

    auto dog = FindGameSessionById(map_id)->AddDog(name);
    return UnitParameters{game_session, dog};
}

const Map* Game::FindMap(const Map::Id& id) const noexcept  {
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

shared_ptr<GameSession> Game::FindGameSessionById(const model::Map::Id& map_id) const {
    if (auto it = map_id_to_session_.find(map_id); it != map_id_to_session_.end()) {
        return it->second;
    }

    return nullptr;
}

const Game::Maps& Game::GetMaps() const noexcept {
    return maps_;
}

const double Game::GetSpeed() const noexcept {
    return def_speed;
}
} // namespace model
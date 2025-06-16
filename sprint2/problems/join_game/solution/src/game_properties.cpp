#include <stdexcept>

#include "game_properties.h"

namespace model {
using namespace std::literals;

using std::shared_ptr;
using std::string;

//__________GameSession__________
shared_ptr<Dog> GameSession::AddDog(const string& name) {  
    dogs_.push_back(model::Dog(name, next_id++));     
    return std::make_shared<model::Dog>(dogs_.back());
}

Map::Id GameSession::GetMapId() const {
    return map_.GetId();
}

const std::vector<Dog>& GameSession::GetDogs() const {
    return dogs_;
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

const Game::Maps& Game::GetMaps() const noexcept{
    return maps_;
}

const Map* Game::FindMap(const Map::Id& id) const noexcept  {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}
Map* Game::FindMap(const Map::Id &id) {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

UnitParameters Game::PrepareUnitParameters(const model::Map::Id& map_id, const std::string& name) {
    auto game_session = FindGameSessionById(map_id);

    if(!game_session) {
        game_session = std::make_shared<GameSession>(sessions_.emplace_back(*FindMap(map_id)));
        map_id_to_session_.insert({map_id, game_session});
    }

    auto dog = FindGameSessionById(map_id)->AddDog(name);
    return UnitParameters{game_session, dog};
}

shared_ptr<GameSession> Game::FindGameSessionById(const model::Map::Id& map_id) const {
    if (auto it = map_id_to_session_.find(map_id); it != map_id_to_session_.end()) {
        return it->second;
    }

    return nullptr;
}

} // namespace model
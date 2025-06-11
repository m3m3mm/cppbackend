#include <stdexcept>

#include "game_properties.h"

namespace model {
using namespace std::literals;

using std::shared_ptr;
using std::string;

const static SpeedUnit DEF_SPEED{0.0,0.0};
const static double MILLISECOND_PER_SECOND = 1000.;

//__________GameSession__________
shared_ptr<Dog> GameSession::AddDog(const string& name) {  
    dogs_.emplace_back(std::make_shared<Dog>(name, next_id++, GetDefPosition()));     
    return dogs_.back();
}

void GameSession::MoveUnits(const std::chrono::milliseconds& time_delta) const {
    for(const auto dog : dogs_) {
        const auto& position = dog->GetCoord();
        const auto& speed = dog->GetSpeed();

        CoordUnit reqested_position = {position.x + speed.horizontal * time_delta.count() / MILLISECOND_PER_SECOND,
                                       position.y + speed.vertical * time_delta.count() / MILLISECOND_PER_SECOND};

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

Map::Id GameSession::GetMapId() const {
    return map_.GetId();
}

const Map& GameSession::GetMap() const {
    return map_;
}

const std::vector<std::shared_ptr<Dog>>& GameSession::GetDogs() const {
    return dogs_;
}

double GameSession::ComputeDistance(CoordUnit lhs, CoordUnit rhs) const {
    double distance = std::pow(rhs.x - lhs.x, 2) + std::pow(rhs.y - lhs.y, 2);

    return std::sqrt(distance);
}

std::optional<CoordUnit> GameSession::ComputeAllowedPosition(CoordUnit cur_pos, CoordUnit new_pos) const {
    const auto roads = FindCandidateRoads(cur_pos);

    if(roads.empty()) {
        return std::nullopt;
    }

    CoordUnit max_allowed_pos = cur_pos;
    double max_distance = ComputeDistance(cur_pos, cur_pos);

    for (const auto& road : roads) {
        if(road->IsCurrectPos(new_pos)) {
            return new_pos;
        }

        CoordUnit allowed_pos = road->Clamp(new_pos);
        double distance = ComputeDistance(cur_pos, allowed_pos);

        if (distance > max_distance) {
            max_allowed_pos = allowed_pos;
            max_distance = distance;
        }
    }

    return max_allowed_pos;
}

std::vector<const Road*> GameSession::FindCandidateRoads(CoordUnit coord) const {
    std::vector<const Road*> result;

    if(auto road = map_.FindRoad(true, coord.y); road 
                                                 && road->IsCurrectPos(coord)) {                                                                                   
        result.push_back(road);
    }

    if(auto road = map_.FindRoad(false, coord.x); road
                                                  && road->IsCurrectPos(coord)) {                                                                          
        result.push_back(road);
    }

    return result;
}

const Road &GameSession::GenerateRandomRoad()
{
    std::random_device random_device_;
    std::mt19937_64 generator(random_device_());
    std::uniform_int_distribution<> dist(0, map_.GetRoads().size() - 1);

    return *map_.GetRoads()[dist(generator)];
}

CoordUnit GameSession::GenerateRandomPosition() {
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
    
    return IsHorizontal ? CoordUnit{gen_coord, position_road}
                        : CoordUnit{position_road, gen_coord};
}

CoordUnit GameSession::GetDefPosition() {
    auto road = map_.GetRoads()[0];
    double start_x = static_cast<double>(road->GetStart().x);
    double start_y = static_cast<double>(road->GetStart().y);
    
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

UnitParameters Game::PrepareUnitParameters(const Map::Id& map_id, const string& name) {
    auto game_session = FindGameSessionById(map_id);

    if(!game_session) {
        game_session = (sessions_.emplace_back(std::make_shared<GameSession>(*FindMap(map_id))));
        map_id_to_session_.insert({map_id, game_session});
    }

    auto dog = FindGameSessionById(map_id)->AddDog(name);
    return UnitParameters{game_session, dog};
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
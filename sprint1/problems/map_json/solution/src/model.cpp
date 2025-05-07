#include "model.h"

#include <stdexcept>

namespace model {
using namespace std::literals;

void Map::AddOffice(Office office) {
    const auto& id = office.GetId();
    if (warehouse_id_to_index_.contains(id)) {
        throw std::invalid_argument("Duplicate office");
    }
    
    const size_t index = offices_.size();
    offices_.push_back(std::move(office));
    warehouse_id_to_index_[id] = index;
}

void Game::AddMap(Map map) {
    const auto& id = map.GetId();
    if (map_id_to_index_.contains(id)) {
        throw std::invalid_argument("Duplicate map");
    }
    
    const size_t index = maps_.size();
    maps_.push_back(std::move(map));
    map_id_to_index_[id] = index;
}

}  // namespace model

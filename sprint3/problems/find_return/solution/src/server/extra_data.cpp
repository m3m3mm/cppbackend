#include "extra_data.h"

void extra_data::LootTypes::SetTypes(const model::Map::Id& map_id, const boost::json::array& types) {
    types_to_map_.insert({map_id, types});
}

const boost::json::array* extra_data::LootTypes::GetTypes(const model::Map::Id& map_id) const {
    if(auto it = types_to_map_.find(map_id); it != types_to_map_.end()) {
        return &it->second;
    }

    return nullptr;
}

#pragma once

#include <boost/json.hpp>

#include <unordered_map>
#include <string>

#include "../model/static_object_prorerties.h"
#include "../tagged.h"

namespace extra_data {
class LootTypes {
public:
    void SetTypes(const model::Map::Id& map_id, const boost::json::array& types);
    const boost::json::array* GetTypes(const model::Map::Id& map_id) const;
private:
    using MapIdHasher = util::TaggedHasher<model::Map::Id>;
    using TypesToMap = std::unordered_map<model::Map::Id, boost::json::array,MapIdHasher>;

    TypesToMap types_to_map_;
};
}//namespace entra_data
#pragma once

#include <boost/serialization/vector.hpp>

#include <string>
#include <stdexcept>
#include <vector>

#include "../dynamic_object_properties.h"
#include "../game_properties.h"

namespace model {
    template<typename Archive>
    void serialize(Archive& ar, CoordObject& pos, [[maybe_unused]] const unsigned version) {
        ar& pos.x;
        ar& pos.y;
    }

    template<typename Archive>
    void serialize(Archive& ar, SpeedUnit& speed, [[maybe_unused]] const unsigned version) {
        ar& speed.horizontal;
        ar& speed.vertical;
    }
}//namespace model

namespace serialization {
class LootRepr {
public:
    LootRepr() = default;
    LootRepr(const model::Loot& loot);

    [[nodiscard]] model::Loot Restore() const;

    template<typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& id_;
        ar& type_;
        ar& cost_;
        ar& position_;
        ar& is_picked_up_;
    }
private:
    size_t id_ = 0;
    size_t type_ = 0;
    size_t cost_ = 0;
    model::CoordObject position_{0.0, 0.0};
    bool is_picked_up_ = false;
};

class DogRepr {
public:
    using Bag = std::vector<LootRepr>;
    DogRepr() = default;
    DogRepr(const model::Dog& dog);

    [[nodiscard]] model::Dog Restore() const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& coord_;
        ar& prev_coord_;
        ar& speed_;
        ar& dir_;
        ar& bag_;
        ar& name_;
        ar& bag_capacity_;
        ar& score_;
        ar& id_;
    }
private:
    model::CoordObject coord_ {0.0, 0.0};
    model::CoordObject prev_coord_ {0.0, 0.0};
    model::SpeedUnit speed_ {0.0, 0.0};
    model::Direction dir_ = model::Direction::U;
    Bag bag_;
    std::string name_= "";
    size_t bag_capacity_ = 0;
    size_t score_ = 0;
    size_t id_ = 0;
};

class GameSessionRepr {
public:
    using Dogs = std::vector<DogRepr>;
    using LostObjects = std::vector<LootRepr>;
    using SessionDogs = model::GameSession::Dogs;
    using SessionLoot = model::GameSession::LostObjects;

    GameSessionRepr() = default;
    GameSessionRepr(const model::GameSession& session);

    [[nodiscard]] model::Map::Id RestoreMapId() const;
    [[nodiscard]] SessionDogs RestoreDogs() const;
    [[nodiscard]] SessionLoot RestoreLoot() const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& dogs_;
        ar& lost_objects_;
        ar& map_id_;
    }   
private:
    Dogs dogs_;
    LostObjects lost_objects_;
    std::string map_id_;
};
}//namespace serialization
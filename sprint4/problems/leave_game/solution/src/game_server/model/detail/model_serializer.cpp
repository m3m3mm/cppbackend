#include <algorithm>

#include "model_serializer.h"

namespace serialization {
//_________LootRepr_________
LootRepr::LootRepr(const model::Loot& loot)
    : id_(loot.GetId())
    , type_ (loot.GetType())
    , cost_ (loot.GetCost())
    , position_(loot.GetPosition())
    , is_picked_up_(loot.IsPickedUp()) {
}

model::Loot LootRepr::Restore() const {
    return model::Loot(id_, type_, cost_, position_, is_picked_up_);
}

//_________DogRepr_________
DogRepr::DogRepr(const model::Dog& dog) 
    : coord_(dog.GetCoord())
    , prev_coord_(dog.GetPrevCoord())
    , speed_(dog.GetSpeed())
    , dir_(dog.GetDirection())
    , name_(dog.GetName())
    , bag_capacity_(dog.GetBagCapacity())
    , score_(dog.GetScore())
    , id_(dog.GetId()) {
    
    auto bag = dog.GetBag();
    if(!bag.empty()) {
        bag_.resize(bag.size());
        std::transform(bag.begin(), bag.end(), bag_.begin(), [&](const model::Loot& loot) {
            return LootRepr(loot);
        });
    }
}

model::Dog DogRepr::Restore() const {
    //передаем старые координаты собаки
    model::Dog dog(prev_coord_, name_, id_, bag_capacity_);
         
    //вызываем метод Move, чтобы старые и новые координаты встали на свои места
    dog.Move(coord_);
    dog.UpdateState(speed_, dir_);
    dog.AddScore(score_);
    
    bool success;
    if(!bag_.empty()) {
        success = std::all_of(bag_.begin(), bag_.end(), [&](const serialization::LootRepr& loot) {
            return dog.AddLoot(loot.Restore());
        });
    }
    
    if(!success) {
        throw std::runtime_error("Failed to put bag content");
    }
    
    return dog;
}

//_________GameSessionRepr_________
GameSessionRepr::GameSessionRepr(const model::GameSession& session) 
    : map_id_(*session.GetMapId()) {

    auto dogs = session.GetDogs();
    if(!dogs.empty()) {
        dogs_.resize(dogs.size());

        std::transform(dogs.begin(), dogs.end(), dogs_.begin(), [&](const model::DogPtr& dog) {
            return DogRepr(*dog);
        });
    }
    
    auto lost_objects = session.GetLoot();
    if(!lost_objects.empty()) {
        lost_objects_.resize(lost_objects.size());

        std::transform(lost_objects.begin(), lost_objects.end(), lost_objects_.begin(), [&](const model::Loot& loot) {
            return LootRepr(loot);
        });
    }
}

model::Map::Id GameSessionRepr::RestoreMapId() const  {
    return model::Map::Id(map_id_);
}

GameSessionRepr::SessionDogs GameSessionRepr::RestoreDogs() const {
    std::vector<model::DogPtr> result;

    if(!dogs_.empty()) {
        result.resize(dogs_.size());

        std::transform(dogs_.begin(), dogs_.end(), result.begin(), [&](const DogRepr& dog_repr) {
            return std::make_shared<model::Dog>(dog_repr.Restore());
        });
    }
    
    return result;
}

GameSessionRepr::SessionLoot GameSessionRepr::RestoreLoot() const {
    std::vector<model::Loot> result;

    if(!lost_objects_.empty()) {
        result.resize(lost_objects_.size());

        std::transform(lost_objects_.begin(), lost_objects_.end(), result.begin(), [&](const LootRepr& loot) {
            return loot.Restore();
        });
    }

    return result;
}
} // namespace serialization

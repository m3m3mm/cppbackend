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
    
    for(const auto& loot : dog.GetBag()) {
        bag_.push_back(LootRepr(loot));
    }
}

model::Dog DogRepr::Restore() const {
    //передаем старые координаты собаки
    model::Dog dog(name_, id_, bag_capacity_, prev_coord_);
         
    //вызываем метод Move, чтобы старые и новые координаты встали на свои места
    dog.Move(coord_);
    dog.UpdateState(speed_, dir_);
    dog.AddScore(score_);
    for(const auto& loot : bag_) {
        if(!dog.AddLoot(loot.Restore())) {
            throw std::runtime_error("Failed to put bag content");
        }
    }

    return dog;
}

//_________GameSessionRepr_________
GameSessionRepr::GameSessionRepr(const model::GameSession& session) 
    : map_id_(*session.GetMapId()) {
    for(const auto& dog : session.GetDogs()) {
        dogs_.push_back(DogRepr(*dog));
    }

    for(const auto& loot : session.GetLoot()) {
        lost_objects_.push_back(LootRepr(loot));
    }
}

model::Map::Id GameSessionRepr::RestoreMapId() const  {
    return model::Map::Id(map_id_);
}

GameSessionRepr::SessionDogs GameSessionRepr::RestoreDogs() const {
    std::vector<model::DogPtr> result;

    for(const auto& dog : dogs_) {
        result.push_back(std::make_shared<model::Dog>(dog.Restore()));
    }

    return result;
}

GameSessionRepr::SessionLoot GameSessionRepr::RestoreLoot() const {
    std::vector<model::Loot> result;

    for(const auto& loot : lost_objects_) {
        result.push_back(loot.Restore());
    }

    return result;
}
} // namespace serialization

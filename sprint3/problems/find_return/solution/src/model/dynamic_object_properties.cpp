#include "dynamic_object_properties.h"

namespace model {

 //__________Loot__________
Loot::Loot(size_t id, size_t type, CoordObject position)
    : id_(id)
    , type_(type)
    , position_(position) {
}

void Loot::MarkPikedUp() const {
    is_picked_up_ = true;
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

bool Loot::IsPickedUp() const {
    return is_picked_up_;
}

//__________Dog__________
Dog::Dog(const std::string& name, size_t id, size_t bag_capacity, CoordObject coord) 
    : coord_(coord)
    , name_(name)
    , id_(id) {
    bag_.reserve(bag_capacity);
}

void Dog::UpdateState(SpeedUnit speed, Direction dir) const {
    dir_ = dir;
    speed_ = speed;
}

void Dog::Move(CoordObject coord) const {
    prev_coord_ = coord_;  
    coord_ = coord;
}

void Dog::TryPickUpLoot(const Loot& loot) {
    if(!IsFullBag() && !loot.IsPickedUp()) {
        bag_.push_back(loot);
        loot.MarkPikedUp();
    }
}

void Dog::LayOutLoot() {
    bag_.clear();
}

std::string Dog::GetName() const {
    return name_;
}

size_t Dog::GetId() const {
    return id_;
}

CoordObject Dog::GetCoord() const {
    return coord_;
}

CoordObject Dog::GetPrevCoord() const {
    return prev_coord_;
}

SpeedUnit Dog::GetSpeed() const {
    return speed_;
}

Direction Dog::GetDirection() const {
    return dir_;
}

size_t Dog::GetBagCapacity() const {
    return bag_.capacity();
}

std::vector<Loot>& Dog::GetBag() const {
    return bag_;
}

std::string Dog::GetDirectionToString() const {
    switch (dir_) {
        case Direction::U :
            return "U";

        case Direction::D :
            return "D";

        case Direction::L :
            return "L";

        case Direction::R :
            return "R";

        default:
            return "";
    }
}

bool Dog::IsFullBag() const {
    return bag_.size() == bag_.capacity();
}
} // namespace model
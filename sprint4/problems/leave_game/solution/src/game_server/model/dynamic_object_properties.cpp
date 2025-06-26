#include "dynamic_object_properties.h"

namespace model {
 //__________Loot__________
Loot::Loot(size_t id, size_t type, size_t cost, CoordObject position, bool is_picked_up)
    : id_(id)
    , type_(type)
    , cost_(cost)
    , position_(position)
    , is_picked_up_(is_picked_up) {
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

size_t Loot::GetCost() const {
    return cost_;
}

CoordObject Loot::GetPosition() const {
    return position_;
}

bool Loot::IsPickedUp() const {
    return is_picked_up_;
}

//__________Dog__________
Dog::Dog(CoordObject coord, const std::string& name, size_t id, size_t bag_capacity) 
    : coord_(coord)
    , prev_coord_(coord)
    , name_(name)
    , id_(id) {
    bag_.reserve(bag_capacity);
}

void Dog::UpdateState(SpeedUnit speed, Direction dir) {
    dir_ = dir;
    speed_ = speed;
}

void Dog::Move(CoordObject coord) {
    prev_coord_ = coord_;  
    coord_ = coord;
}

void Dog::LayOutLoot() {
    while(!bag_.empty()) {
        score_ += bag_.back().GetCost();
        bag_.pop_back();
    };
}

void Dog::AddScore(size_t score) {
    score_ = score;
}

bool Dog::AddLoot(const Loot& loot) {
    if(!IsFullBag()) {
        bag_.push_back(loot);
        loot.MarkPikedUp();
        return true;
    }

    return false;
}

void Dog::TryPickUpLoot(const Loot& loot) {
    if(!IsFullBag() && !loot.IsPickedUp()) {
        bag_.push_back(loot);
        loot.MarkPikedUp();
    }
}

void Dog::SetTimeInGame(const std::chrono::milliseconds& time) {
    time_in_game_ = time;
}

void Dog::SetInactiveTime(const std::chrono::milliseconds& time) {
    inactive_time_ = time;
}

const std::string& Dog::GetName() const {
    return name_;
}

const Dog::Bag& Dog::GetBag() const {
    return bag_;
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

size_t Dog::GetScore() const {
    return score_;
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

std::chrono::milliseconds Dog::GetTimeInGame() const {
    return time_in_game_;
}

std::chrono::milliseconds Dog::GetInactiveTime() const {
    return inactive_time_;
}

bool Dog::IsFullBag() const {
    return bag_.size() == bag_.capacity();
}
} // namespace model
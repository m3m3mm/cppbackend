#pragma once

#include <string>
#include <vector>

namespace model {
const static double DEFAULT_SPEED = 1.;
const static size_t DEFAULT_BAG_CAPACITY = 3;

struct SpeedUnit {
    double horizontal;
    double vertical;
}; 

struct CoordObject {
    double x; 
    double y;

    bool operator==(CoordObject other) const {
        return x == other.x && y == other.y;
    }
    
    bool operator!=(CoordObject other) const {
        return x != other.x && y != other.y;
    }

    bool operator>(CoordObject other) {
        return x > other.x && y > other.y;
    }

    bool operator>=(CoordObject other) {
        return x >= other.x && y >= other.y;
    }

    bool operator<(CoordObject other) {
        return x < other.x && y < other.y;
    }

    bool operator<=(CoordObject other) {
        return x <= other.x && y <= other.y;
    }
};

enum class Direction {
    U,
    D,
    R,
    L,
    STOP
};

class Loot {
public:
    Loot(size_t id, size_t type, size_t cost, CoordObject position);

    void MarkPikedUp() const;

    size_t GetId() const;
    size_t GetType() const;
    size_t GetCost() const;
    CoordObject GetPosition() const;

    bool IsPickedUp() const;
private:
    size_t id_;
    size_t type_;
    size_t cost_;
    CoordObject position_;
    mutable bool is_picked_up_ = false;
};

class Dog {
public:
    using Bag = std::vector<Loot>;

    explicit Dog(const std::string& name, size_t id, size_t bag_capacity, CoordObject coord);

    void UpdateState(SpeedUnit speed, Direction dir) const;
    void Move(CoordObject coord) const;
    void TryPickUpLoot(const Loot& loot);
    void LayOutLoot();

    std::string GetName() const;
    size_t GetId() const;
    CoordObject GetCoord() const;
    CoordObject GetPrevCoord() const;
    SpeedUnit GetSpeed() const;
    Direction GetDirection() const;
    size_t GetBagCapacity() const;
    size_t GetScore() const;
    std::vector<Loot>& GetBag() const;
    std::string GetDirectionToString() const;
private:
    mutable CoordObject coord_ {0.0, 0.0};
    mutable CoordObject prev_coord_ {0.0, 0.0};
    mutable SpeedUnit speed_ {0.0, 0.0};
    mutable Direction dir_ = Direction::U;
    mutable Bag bag_;
    std::string name_= "";
    size_t score_ = 0;
    size_t id_ = 0;

    bool IsFullBag() const;
};
}//namespace model
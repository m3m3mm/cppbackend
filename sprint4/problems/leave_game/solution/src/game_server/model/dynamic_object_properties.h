#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace model {
const static double DEFAULT_SPEED = 1.;
const static size_t DEFAULT_BAG_CAPACITY = 3;

struct SpeedUnit {
    double horizontal;
    double vertical;

    constexpr auto operator<=>(const SpeedUnit&) const = default;
}; 

struct CoordObject {
    double x; 
    double y;

    constexpr auto operator<=>(const CoordObject&) const = default;
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
    Loot() = default;
    Loot(size_t id, size_t type, size_t cost, CoordObject position, bool is_picked_up = false);

    void MarkPikedUp() const;

    size_t GetId() const;
    size_t GetType() const;
    size_t GetCost() const;
    CoordObject GetPosition() const;

    bool IsPickedUp() const;

    [[nodiscard]] auto operator<=>(const Loot&) const = default;
private:
    size_t id_;
    size_t type_;
    size_t cost_;
    CoordObject position_;
    mutable bool is_picked_up_;
};

class Dog {
public:
    using Bag = std::vector<Loot>;
    Dog() = default;
    explicit Dog(CoordObject coord, const std::string& name, size_t id, size_t bag_capacity);

    void AddScore(size_t score);
    bool AddLoot(const Loot& loot);

    void UpdateState(SpeedUnit speed, Direction dir);
    void Move(CoordObject coord);
    void LayOutLoot();
    void TryPickUpLoot(const Loot& loot);

    void SetTimeInGame(const std::chrono::milliseconds& time);
    void SetInactiveTime(const std::chrono::milliseconds& time);

    const std::string& GetName() const;
    const std::vector<Loot>& GetBag() const;
    size_t GetId() const;
    CoordObject GetCoord() const;
    CoordObject GetPrevCoord() const;
    SpeedUnit GetSpeed() const;
    Direction GetDirection() const;
    size_t GetBagCapacity() const;
    size_t GetScore() const;
    std::string GetDirectionToString() const;
    std::chrono::milliseconds GetTimeInGame() const;
    std::chrono::milliseconds GetInactiveTime() const;
private:
    CoordObject coord_ {0.0, 0.0};
    CoordObject prev_coord_ {0.0, 0.0};
    SpeedUnit speed_ {0.0, 0.0};
    Direction dir_ = Direction::U;
    Bag bag_;
    std::string name_= "";
    size_t score_ = 0;
    size_t id_ = 0;

    std::chrono::milliseconds time_in_game_{0};
    std::chrono::milliseconds inactive_time_{0};

    bool IsFullBag() const;
};

using DogPtr = std::shared_ptr<Dog>;
using ConstDogPtr = std::shared_ptr<const Dog>;
}//namespace model
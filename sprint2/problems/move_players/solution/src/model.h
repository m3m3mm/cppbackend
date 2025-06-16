#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "tagged.h"

namespace model {
using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

struct CoordUnit {
    double x, y;
};

struct SpeedUnit {
    double horizontal;
    double vertical;
}; 

enum class Direction {
    U,
    D,
    R,
    L,
    STOP
};

class Road {
    struct HorizontalTag {
        HorizontalTag() = default;
    };

    struct VerticalTag {
        VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }

private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const double GetSpeed() const noexcept {
        return dog_speed_;
    } 

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    void AddDogSpeed(double speed) {
        dog_speed_ = speed;
    }

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

    

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    double dog_speed_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

class Dog {
public:
    explicit Dog(const std::string& name, size_t id, CoordUnit coord) 
        : name_(name)
        , id_(id)
        ,coord_(coord) {
    }

    void Move(SpeedUnit speed) const {  
        speed_ = speed;
    }

    std::string GetName() const {
        return name_;
    };

    size_t GetId() const {
        return id_;
    };

    CoordUnit GetCoord() const {
        return coord_;
    }
    SpeedUnit GetSpeed() const {
        return speed_;
    }

    std::string GetDirection() const {
        switch (dir_) {
            case Direction::U :
                return "U";
                break;
            case Direction::D :
                return "D";
                break;
            case Direction::L :
                return "L";
                break;
            case Direction::R :
                return "R";
                break;
            default:
                return "";
                break;
        }
    }
private:
    mutable CoordUnit coord_ {0.0, 0.0};
    mutable SpeedUnit speed_ {0.0, 0.0};
    mutable Direction dir_ = Direction::U;
    std::string name_= "";
    size_t id_ = 0;
};
}  // namespace model

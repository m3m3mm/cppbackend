#pragma once

#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../tagged.h"
#include "dynamic_object_properties.h"

namespace model {
using Dimension = int;
using Coord = Dimension;

const static double MAX_INDENT = 0.4;

struct Bounds {
    double upper;
    double lower;
    double left;
    double right;
};

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

using PairCompareRoad = std::pair<bool, double>;

class ComparatorRoad {
public:
    bool operator()(const PairCompareRoad& lhs, const PairCompareRoad& rhs) const;
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

    Road(HorizontalTag, Point start, Coord end_x) noexcept;
    Road(VerticalTag, Point start, Coord end_y) noexcept;

    CoordObject Clamp (CoordObject new_pos) const;

    Point GetStart() const noexcept;
    Point GetEnd() const noexcept;
    Bounds GetBounds() const;

    bool IsHorizontal() const noexcept;
    bool IsVertical() const noexcept;
    bool IsPositiveOffset() const noexcept;
    bool IsCurrectPos(CoordObject pos) const;

private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept;

    const Rectangle& GetBounds() const noexcept;
private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept;

    const Id& GetId() const noexcept;
    Point GetPosition() const noexcept;
    Offset GetOffset() const noexcept;

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<const Road*>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name, size_t loot_types_count) noexcept;
    
    void AddRoad(const Road& road);
    void AddBuilding(const Building& building);
    void AddOffice(Office office);

    const Map::Roads FindCandidateRoads(CoordObject coord_position) const;

    void SetDogSpeed(double speed);
    void SetBagCapacity(size_t capacity);

    const Id& GetId() const noexcept;
    size_t GetLootTypesCount() const noexcept;
    size_t GetBagCapacity() const noexcept;
    double GetSpeed() const noexcept;
    const std::string& GetName() const noexcept;

    const Buildings& GetBuildings() const noexcept;
    const Roads& GetRoads() const noexcept;

    const Offices& GetOffices() const noexcept;
private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;
    using RoadToPair = std::multimap<PairCompareRoad, const Road, ComparatorRoad>;
    
    Id id_;
    std::string name_;
    size_t loot_types_count_;
    double dog_speed_;
    size_t bag_capacity_;

    RoadToPair road_to_bound_and_dir;
    Roads roads_;
    
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};
}// namespace model

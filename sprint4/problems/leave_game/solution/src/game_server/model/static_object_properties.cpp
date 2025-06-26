#include <stdexcept>

#include "static_object_prorerties.h"

namespace model {
using namespace std::literals;

bool ComparatorRoad::operator()(const PairCompareRoad& lhs, const PairCompareRoad& rhs) const {
    return std::pair(lhs.first, lhs.second) < std::pair(rhs.first, rhs.second); 
}

//__________Road__________
Road::Road(HorizontalTag, Point start, Coord end_x) noexcept 
    : start_{start}
    , end_{end_x, start.y} {
}

Road::Road(VerticalTag, Point start, Coord end_y) noexcept
    : start_{start}
    , end_{start.x, end_y} {
}

CoordObject Road::Clamp(CoordObject new_pos) const  {
    auto bounds = GetBounds();
    
    if(IsHorizontal()) {
        return {std::clamp(new_pos.x, bounds.lower, bounds.upper),
                std::clamp(new_pos.y, bounds.left, bounds.right)};
    } 
    
    return {std::clamp(new_pos.x, bounds.left, bounds.right),
            std::clamp(new_pos.y, bounds.lower, bounds.upper)};
}

Point Road::GetStart() const noexcept {
    return start_;
}

Point Road::GetEnd() const noexcept { 
    return end_;
}

Bounds Road::GetBounds() const {
    Bounds result;

    if(IsHorizontal()) {
        result.left = static_cast<double>(start_.y) - MAX_INDENT;
        result.right = static_cast<double>(start_.y) + MAX_INDENT;

        if(IsPositiveOffset()) {
            result.lower = static_cast<double>(start_.x) - MAX_INDENT;
            result.upper = static_cast<double>(end_.x) + MAX_INDENT;
        } else {
            result.lower = static_cast<double>(end_.x) - MAX_INDENT;
            result.upper = static_cast<double>(start_.x) + MAX_INDENT;
        }
    } else {
        result.left = static_cast<double>(start_.x) - MAX_INDENT;
        result.right = static_cast<double>(start_.x) + MAX_INDENT;

        if(IsPositiveOffset()) {
            result.lower = static_cast<double>(start_.y) - MAX_INDENT;
            result.upper = static_cast<double>(end_.y) + MAX_INDENT;
        } else {
            result.lower = static_cast<double>(end_.y) - MAX_INDENT;
            result.upper = static_cast<double>(start_.y) + MAX_INDENT;
        }
    }
    
    return result;
}

bool Road::IsHorizontal() const noexcept {
    return start_.y == end_.y;
}

bool Road::IsVertical() const noexcept {
    return start_.x == end_.x;
}

bool Road::IsPositiveOffset() const noexcept {
    if(IsHorizontal()) {
        return start_.x < end_.x;
    } 
    
    return start_.y < end_.y;
}

bool Road::IsCurrectPos(CoordObject pos) const {
    auto bounds = GetBounds();

    if(IsHorizontal()) {
        return (pos.x >= bounds.lower && pos.y >= bounds.left)  
                && (pos.x <= bounds.upper && pos.y <= bounds.right);
    } 
    return (pos.y >= bounds.lower && pos.x >= bounds.left)
            && (pos.y <= bounds.upper && pos.x <= bounds.right);
}

//__________Building__________
Building::Building(Rectangle bounds) noexcept
    : bounds_{bounds} {
}

const Rectangle& Building::GetBounds() const noexcept {
    return bounds_;
}

//__________Office_________
Office::Office(Id id, Point position, Offset offset) noexcept
    : id_{std::move(id)}
    , position_{position}
    , offset_{offset} {
}

const Office::Id& Office::GetId() const noexcept {
    return id_;
}

Point Office::GetPosition() const noexcept {
    return position_;
}

Offset Office::GetOffset() const noexcept {
    return offset_;
}

//__________Map__________
Map::Map(Id id, std::string name) noexcept
    : id_(std::move(id))
    , name_(std::move(name)) {
}

void Map::AddRoad(const Road& road) {
    PairCompareRoad key = std::pair<bool, double>(road.IsHorizontal(), road.GetBounds().right);
    const Road* road_address = &road_to_bound_and_dir.insert({key, road})->second;
    roads_.push_back(road_address);
}

void Map::AddBuilding(const Building& building) {
    buildings_.emplace_back(building);
}

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

const Map::Roads Map::FindCandidateRoads(CoordObject coord_position) const {
    Map::Roads result;
    model::Map::RoadToPair::const_iterator it;
    
    it = road_to_bound_and_dir.lower_bound(std::pair(true/*horizontal*/, coord_position.y));
    if(it != road_to_bound_and_dir.end()) {
        const auto [begin, end] = road_to_bound_and_dir.equal_range(it->first);

        for(auto iter = begin; iter != end; ++iter) {
            if(iter->second.IsCurrectPos(coord_position)){
                result.push_back(&iter->second);
            }
        }
    }

    it = road_to_bound_and_dir.lower_bound(std::pair(false/*vertical*/, coord_position.x));
    if(it != road_to_bound_and_dir.end()) {
        const auto [begin, end] = road_to_bound_and_dir.equal_range(it->first);

        for(auto iter = begin; iter != end; ++iter) {
            if(iter->second.IsCurrectPos(coord_position)){
                result.push_back(&iter->second);
            }
        }
    }
    
    return result;
}

void Map::SetDogSpeed(double speed) {
    dog_speed_ = speed;
}

void Map::SetBagCapacity(size_t capacity) {
    bag_capacity_ = capacity;
}

void Map::SerLootUnitCost(size_t cost) {
    types_cost_to_index_.push_back(cost);
}

const Map::Id& Map::GetId() const noexcept {
    return id_;
}

size_t Map::GetTypeCost(size_t type) const {
    return types_cost_to_index_[type];
}

size_t Map::GetLootTypesCount() const noexcept {
    return types_cost_to_index_.size();
}

size_t Map::GetBagCapacity() const noexcept {
    return bag_capacity_;
}

double Map::GetSpeed() const noexcept {
    return dog_speed_;
}

const std::string& Map::GetName() const noexcept {
    return name_;
}

const Map::Buildings& Map::GetBuildings() const noexcept {
    return buildings_;
}

const Map::Roads& Map::GetRoads() const noexcept {
    return roads_;
}

const Map::Offices& Map::GetOffices() const noexcept {
    return offices_;
}
} // namespace model



#define _USE_MATH_DEFINES

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_predicate.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <cmath>
#include <functional>

#include "../src/model/detail/collision_detector.h"

using namespace collision_detector;
using namespace std::literals;

using Catch::Matchers::WithinAbs;

namespace Catch {
const static double ALLOWABLE_ERROR = 1e-10;
template<>
struct StringMaker<collision_detector::GatheringEvent> {
  static std::string convert(collision_detector::GatheringEvent const& value) {
      std::ostringstream tmp;
      tmp << "(" << value.gatherer_id << "," << value.item_id << "," << value.sq_distance << "," << value.time << ")";

      return tmp.str();
  }
};

auto are_equal_events = [](const GatheringEvent& lhs, const GatheringEvent& rhs) {
    return lhs.item_id == rhs.item_id 
        && lhs.gatherer_id == rhs.gatherer_id
        && WithinAbs(lhs.sq_distance, ALLOWABLE_ERROR).match(rhs.sq_distance)
        && WithinAbs(lhs.time, ALLOWABLE_ERROR).match(rhs.time);
};

struct EventsCollisionMatcher : Catch::Matchers::MatcherBase<std::vector<GatheringEvent>> {
    EventsCollisionMatcher(std::vector<GatheringEvent>&& events)
        : events_(std::move(events)) {
            
            std::sort(events_.begin(), events_.end(), 
                [](const GatheringEvent& lhs, const GatheringEvent& rhs) {
                    return lhs.time < rhs.time;
                });
    }

    bool match(const std::vector<GatheringEvent>& other) const override {        
        return std::equal(events_.begin(), events_.end(), other.begin(), other.end(), are_equal_events);
    }

    std::string describe() const override {
        return "Is equal to expected events: "s + Catch::rangeToString(events_);
    }

private:
    std::vector<GatheringEvent> events_;
};

class TestItemGathererProvader : public ItemGathererProviderInterface {
public:
    TestItemGathererProvader(std::vector<collision_detector::Item> items,
                             std::vector<collision_detector::Gatherer> gatherers)
        : items_(items)
        , gatherers_(gatherers) {
    }

    
    size_t ItemsCount() const override {
        return items_.size();
    }
    collision_detector::Item GetItem(size_t idx) const override {
        return items_[idx];
    }
    size_t GatherersCount() const override {
        return gatherers_.size();
    }
    collision_detector::Gatherer GetGatherer(size_t idx) const override {
        return gatherers_[idx];
    }

private:
    std::vector<collision_detector::Item> items_;
    std::vector<collision_detector::Gatherer> gatherers_;
};

const static double gatherer_width = 0.6;
const static double item_width = 0.0;

SCENARIO("No collision detected") {
    double gatherer_width = 0.6;
    double item_width = 0.0;

    WHEN("no items") {
        TestItemGathererProvader provider{
            {}, {{{5.2, 2.1}, {7.8, 1.2}, gatherer_width}, {{0.7, 0.8}, {10, 10}, gatherer_width}, {{-5, 0}, {10, 5}, gatherer_width}}};

        THEN("no events") {
            auto events = FindGatherEvents(provider);
            CHECK(events.empty());
        }
    }

    AND_WHEN("no gatherers") {
        TestItemGathererProvader provider {
            {{{1.1, 0.1}, item_width}, {{5.8, 6.2}, item_width}, {{8.1, 4.6}, item_width}}, {}};
        
        THEN("no events"){
            auto events = FindGatherEvents(provider);
            CHECK(events.empty());
        }
    }

    AND_WHEN("gatherers and item don't intersect") {
        TestItemGathererProvader provider {
            {{{1.1, 0.1}, item_width}, {{5.8, 6.2}, item_width}},
            {{{5.9, 2.1}, {7.8, 1.2}, gatherer_width}, {{4.2, 8.1}, {10.9, 11.2}, gatherer_width}}};
        
        THEN("no events"){
            auto events = FindGatherEvents(provider);
            CHECK(events.empty());
        }
    }

    AND_WHEN("gatherers stay put") {
        TestItemGathererProvader provider {
            {{{1.1, 0.1}, item_width}, {{5.8, 6.2}, item_width}},
            {{{1.1, 0.1}, {1.1, 0.1}, gatherer_width}, {{5.8, 6.2}, {5.8, 6.2}, gatherer_width}}};
 
        THEN("no events") {
            auto events = FindGatherEvents(provider);
            CHECK(events.empty());
        }
    }
}

SCENARIO("Collision detected") {
    WHEN("multiple items on a way of gatherer") {
        TestItemGathererProvader provider {
            {{{0.0, 0.0}, item_width}/*event*/, {{1.0, 0.06}, item_width}/*event*/, {{2.0, 0.25}, item_width}/*event*/,
             {{3.0, 0.31}, item_width}/*event*/,{{4.0, 0.59}, item_width}/*event*/, {{5.0, 0.6}, item_width}/*event*/,
             {{6.0, 0.62}, item_width}/*no event*/, {{7.0, 0.72}, item_width}/*no event*/, {{8.0, -0.02}, item_width}/*event*/},
            {{{0.0, 0.0}, {10.0, 0.0}, gatherer_width}}};

        THEN("gathered items in right order") {
            auto events = collision_detector::FindGatherEvents(provider);
            EventsCollisionMatcher matcher(std::move(events));
            CHECK(matcher.match(std::vector{
                                collision_detector::GatheringEvent{0, 0, 0.0 * 0.0,     0.0},
                                collision_detector::GatheringEvent{1, 0, 0.06 * 0.06,   0.1},
                                collision_detector::GatheringEvent{2, 0, 0.25 * 0.25,   0.2},
                                collision_detector::GatheringEvent{3, 0, 0.31 * 0.31,   0.3},
                                collision_detector::GatheringEvent{4, 0, 0.59 * 0.59,   0.4},
                                collision_detector::GatheringEvent{5, 0, 0.6 * 0.6,     0.5},
                                collision_detector::GatheringEvent{8, 0, -0.02 * -0.02, 0.8}
                                }));
        }
    }

    AND_WHEN("multiple gatherers and one item") {
        TestItemGathererProvader provider {
            {{{0.0, 0.0}, item_width}},
            {{{-5.0, 0.0}, {6.0, 0.0}, gatherer_width}, 
             {{-4.0, 0.0}, {5.0, 0.0}, gatherer_width}, 
             {{-3.0, 0.0}, {4.0, 0.0}, gatherer_width},
             {{-2.0, 0.0}, {3.0, 0.0}, gatherer_width}}};

        THEN("item gathered by faster gatherer") {
            auto events = collision_detector::FindGatherEvents(provider);
        CHECK(events.front().gatherer_id == 3);
        }
    }
}
}  // namespace Catch
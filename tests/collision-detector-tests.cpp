#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "../lib/model/collision_detector.h"

namespace Catch {
template <>
struct StringMaker<model::CollisionEvent> {
  static std::string convert(const model::CollisionEvent& value) {
    std::ostringstream out;
    out << '(' << static_cast<char>(value.type) << ',' << *value.dog_id << ','
        << *value.lost_object_ptr->GetId() << value.sq_distance << ','
        << value.time << ')';
    return out.str();
  }
};
}  // namespace Catch

class IsAscendingRange : public Catch::Matchers::MatcherGenericBase {
 public:
  explicit IsAscendingRange() {}
  IsAscendingRange(IsAscendingRange&&) noexcept = default;

  template <typename Range>
  bool match(Range range) const {
    if (std::empty(range)) {
      return true;
    }
    for (auto elem = std::begin(range); elem != std::prev(std::end(range));
         elem = std::next(elem)) {
      if (*elem > *std::next(elem)) {
        return false;
      }
    }
    return true;
  }

  std::string describe() const override { return "Range is ascending"; }
};

IsAscendingRange IsAscending() { return IsAscendingRange(); }

template <typename Range>
class IsPermutationMatcher : public Catch::Matchers::MatcherGenericBase {
 public:
  explicit IsPermutationMatcher(Range range) : range_(std::move(range)) {
    std::sort(std::begin(range_), std::end(range_));
  }
  IsPermutationMatcher(IsPermutationMatcher&&) noexcept = default;

  template <typename OtherRange>
  bool match(OtherRange other) const {
    using std::begin;
    using std::end;

    std::sort(begin(other), end(other));
    return std::equal(begin(range_), end(range_), begin(other), end(other));
  }

  std::string describe() const override {
    using namespace std::literals;
    return "Is permutation of: "s + Catch::rangeToString(range_);
  }

 private:
  Range range_;
};

template <typename Range>
IsPermutationMatcher<Range> IsPermutation(Range&& range) {
  return IsPermutationMatcher<Range>(std::forward<Range>(range));
}
/*
namespace model {
class ItemGathererProviderImpl : public ItemGathererProvider {
 public:
  using Items = std::vector<Item>;
  using Gatherers = std::vector<Gatherer>;

  ItemGathererProviderImpl(Items items, Gatherers gatherers)
      : items_(std::move(items)), gatherers_(std::move(gatherers)) {}

  size_t ItemsCount() const override { return items_.size(); }

  Item GetItem(size_t idx) const override { return items_.at(idx); }

  size_t GatherersCount() const override { return gatherers_.size(); }

  Gatherer GetGatherer(size_t idx) const override { return gatherers_.at(idx); }

 private:
  Items items_;
  Gatherers gatherers_;
};
}  // namespace model

SCENARIO("Collision detection") {
  using namespace std::literals;
  using namespace model;
  WHEN("no items") {
    ItemGathererProviderImpl provider{
        {},
        {{{1, 2}, {4, 2}, 5.}, {{0, 0}, {10, 10}, 5.}, {{-5, 0}, {10, 5}, 5.}}};
    THEN("No events") {
      auto events = FindGatherEvents(provider);
      CHECK(events.empty());
    }
  }
  WHEN("no gatherers") {
    ItemGathererProviderImpl provider{
        {{{1, 2}, 5.}, {{0, 0}, 5.}, {{-5, 0}, 5.}}, {}};
    THEN("No events") {
      auto events = FindGatherEvents(provider);
      CHECK(events.empty());
    }
  }
  WHEN("multiple items on a way of gatherer") {
    ItemGathererProviderImpl provider{{
                                          {{9, 0.27}, .1},
                                          {{8, 0.24}, .1},
                                          {{7, 0.21}, .1},
                                          {{6, 0.18}, .1},
                                          {{5, 0.15}, .1},
                                          {{4, 0.12}, .1},
                                          {{3, 0.09}, .1},
                                          {{2, 0.06}, .1},
                                          {{1, 0.03}, .1},
                                          {{0, 0.0}, .1},
                                          {{-1, 0}, .1},
                                      },
                                      {
                                          {{0, 0}, {10, 0}, 0.1},
                                      }};
    THEN("Gathered items in right order") {
      auto events = FindGatherEvents(provider);
      CHECK_THAT(events, IsAscending());
      CHECK_THAT(events, IsPermutationMatcher(std::vector{
                             CollisionEvent{9, 0, 0. * 0., 0.0},
                             CollisionEvent{8, 0, 0.03 * 0.03, 0.1},
                             CollisionEvent{7, 0, 0.06 * 0.06, 0.2},
                             CollisionEvent{6, 0, 0.09 * 0.09, 0.3},
                             CollisionEvent{5, 0, 0.12 * 0.12, 0.4},
                             CollisionEvent{4, 0, 0.15 * 0.15, 0.5},
                             CollisionEvent{3, 0, 0.18 * 0.18, 0.6},
                         }));
    }
  }
  WHEN("multiple gatherers and one item") {
    ItemGathererProviderImpl provider{
        {
            {{0, 0}, 0.},
        },
        {
            {{-5, 0}, {5, 0}, 1.},
            {{0, 1}, {0, -1}, 1.},
            {{-10, 10}, {101, -100}, 0.5},  // <-- that one
            {{-100, 100}, {10, -10}, 0.5},
        }};
    THEN("Item gathered by faster gatherer") {
      auto events = FindGatherEvents(provider);
      CHECK(events.front().gatherer_id == 2);
    }
  }
  WHEN("Gatherers stay put") {
    ItemGathererProviderImpl provider{{
                                          {{0, 0}, 10.},
                                      },
                                      {{{-5, 0}, {-5, 0}, 1.},
                                       {{0, 0}, {0, 0}, 1.},
                                       {{-10, 10}, {-10, 10}, 100}}};
    THEN("No events detected") {
      auto events = FindGatherEvents(provider);

      CHECK(events.empty());
    }
  }
}
*/

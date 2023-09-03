#pragma once

#include <chrono>
#include <string>
#include <vector>

#include "../util/tagged.h"
#include "geometry.h"
#include "lost_object.h"
#include "map.h"
#include "road.h"

namespace model {

// Описывает объект собаки, которым управляет игрок.
class Dog {
 public:
  using Id = util::Tagged<std::uint32_t, Dog>;
  using Bag = std::vector<LostObject>;
  using Milliseconds = std::chrono::milliseconds;

  explicit Dog(Id id, std::string name,
               const std::pair<Point, const Road*>& pos,
               std::uint32_t bag_capacity);

  const Id& GetId() const noexcept;

  const std::string& GetName() const noexcept;

  Dimension GetWidth() const noexcept;

  Point GetCurrentPosition() const noexcept;

  Point GetPreviousPosition() const noexcept;

  Speed GetSpeed() const noexcept;

  Direction GetDirection() const noexcept;

  Road GetCurrentRoad() const noexcept;

  const Bag& GetBag() const noexcept;

  std::uint32_t GetScore() const noexcept;

  // Кладет предмет в рюкзак при условии, что рюкзак не заполнен. Если рюкзак
  // заполнен, то возвращает false.
  bool PutInBag(const LostObject& lost_object);

  // Обновляет позицию собаки.
  void UpdatePosition(const Map* map, Milliseconds time_delta);

  // Очищает рюкзак, и увеличивает поле score_.
  void HandOverLoot();

  void SetSpeed(const Speed& speed);

  void SetDirection(const Direction& direction);

 private:
  void SetPosition(const Point& pos);

  void SetRoad(const Road& road);

  // Находит границу дороги, исходя из направления собаки, и меняет ее позицию.
  void SetBoundaries(const Point& unchanged_point,
                     const std::pair<Point, Point>& pos);

  Id id_;
  std::string name_;
  Dimension width_ = 0.6;
  Point curr_pos_{0, 0};
  Point prev_pos_{0, 0};
  Speed speed_{0, 0};
  Direction direction_{Direction::kNorth};
  Road curr_road_;
  Bag bag_;
  std::uint32_t score_ = 0;
};

}  // namespace model
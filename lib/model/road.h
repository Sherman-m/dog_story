#pragma once

#include "geometry.h"

namespace model {

// Описывает объект дорога в игре.
class Road {
 private:
  struct HorizontalTag {
    HorizontalTag() = default;
  };

  struct VerticalTag {
    VerticalTag() = default;
  };

 public:
  constexpr static HorizontalTag HORIZONTAL{};
  constexpr static VerticalTag VERTICAL{};

  Road() = default;
  explicit Road(HorizontalTag, const Point& start_pos,
                Coord end_pos_x) noexcept;
  explicit Road(VerticalTag, const Point& start_pos, Coord end_pos_y) noexcept;

  bool IsHorizontal() const noexcept;
  bool IsVertical() const noexcept;

  Point GetStartPosition() const noexcept;
  Point GetEndPosition() const noexcept;

  Dimension GetWidth() const noexcept;

  // Проверяет, находится ли точка в пределах дороги.
  bool IsInsideRoad(const Point& pos) const;

 private:
  Point start_pos_;
  Point end_pos_;
  Dimension width_{0.8};
};

struct RoadHasher {
  std::size_t operator()(const Road& road) const;
};

bool operator==(const Road& road1, const Road& road2);

}  // namespace model
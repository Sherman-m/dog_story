#include "road.h"

namespace model {

Road::Road(Road::HorizontalTag, const Point& start_pos,
           Coord end_pos_x) noexcept
    : start_pos_(start_pos), end_pos_({end_pos_x, start_pos_.y}) {}
Road::Road(Road::VerticalTag, const Point& start_pos, Coord end_pos_y) noexcept
    : start_pos_(start_pos), end_pos_({start_pos.x, end_pos_y}) {}

bool Road::IsHorizontal() const noexcept { return start_pos_.y == end_pos_.y; }

bool Road::IsVertical() const noexcept { return start_pos_.x == end_pos_.x; }

Point Road::GetStartPosition() const noexcept { return start_pos_; }

Point Road::GetEndPosition() const noexcept { return end_pos_; }

Dimension Road::GetWidth() const noexcept { return width_; }

// Против часовой стрелке находит точки, описывающие дорогу как прямоугольник с
// помощью четырех координат, начиная с левого нижнего угла. Затем с помощью
// функции PseudoscalarProductOfVectors считает псведоскалярное произведение для
// определения того, лежит ли точка pos левее каждой стороны прямоугольника.
// Если результат функции PseudoscalarProductOfVectors < 0, значит точка pos
// лежит правее стороны прямоугольника, а значит находится снаружи
// прямоугольника.
bool Road::IsInsideRoad(const model::Point& pos) const {
  auto half_road_width = width_ / 2;
  auto p1 = Point(std::min(start_pos_.x, end_pos_.x) - half_road_width,
                  std::min(start_pos_.y, end_pos_.y) - half_road_width);
  auto p2 = Point(std::max(start_pos_.x, end_pos_.x) + half_road_width,
                  std::min(start_pos_.y, end_pos_.y) - half_road_width);
  auto p3 = Point(std::max(start_pos_.x, end_pos_.x) + half_road_width,
                  std::max(start_pos_.y, end_pos_.y) + half_road_width);
  auto p4 = Point(std::min(start_pos_.x, end_pos_.x) - half_road_width,
                  std::max(start_pos_.y, end_pos_.y) + half_road_width);
  return PseudoscalarProductOfVectors(pos, p1, p2) >= 0 &&
         PseudoscalarProductOfVectors(pos, p2, p3) >= 0 &&
         PseudoscalarProductOfVectors(pos, p3, p4) >= 0 &&
         PseudoscalarProductOfVectors(pos, p4, p1) >= 0;
}

std::size_t RoadHasher::operator()(const model::Road& road) const {
  std::hash<int> hasher;
  Point road_start_pos = road.GetStartPosition();
  Point road_end_pos = road.GetEndPosition();
  const int first_prime_number = 137;
  const int second_prime_number = 149;
  std::size_t hash1 =
      hasher(static_cast<int>(road_start_pos.x) * first_prime_number +
             static_cast<int>(road_start_pos.y) * first_prime_number);
  std::size_t hash2 =
      hasher(static_cast<int>(road_end_pos.x) * first_prime_number +
             static_cast<int>(road_end_pos.y) * second_prime_number);
  return hasher(hash1 ^ hash2 << 1);
}

bool operator==(const model::Road& road1, const Road& road2) {
  return ((road1.GetStartPosition() == road2.GetStartPosition() &&
           road1.GetEndPosition() == road2.GetEndPosition()) ||
          road1.GetEndPosition() == road2.GetStartPosition() &&
              road1.GetStartPosition() == road2.GetStartPosition());
}

}  // namespace model
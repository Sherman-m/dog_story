#pragma once

#include <cmath>
#include <limits>
#include <string>
#include <unordered_map>

namespace model {

// Здесь находится описание тривиальных объектов игры, таких как координаты,
// точки, прямоугольники и т. д.

using namespace std::literals;

using Dimension = double;
using Coord = Dimension;

struct Point {
  Coord x, y;
};

bool operator==(const Point& p1, const Point& p2);
bool operator!=(const Point& p1, const Point& p2);

struct PointHasher {
  std::size_t operator()(const Point& point) const;
};

struct Size {
  Dimension width, height;
};

struct Rectangle {
  Point pos;
  Size size;
};

struct Offset {
  Dimension dx, dy;
};

struct Speed {
  Dimension sx, sy;
};

enum class Direction : char {
  kNorth = 'U',
  kSouth = 'D',
  kWest = 'L',
  kEast = 'R'
};

// Считает псевдоскалярное произведение двух векторов.
double PseudoscalarProductOfVectors(const Point& target_point,
                                    const Point& point1, const Point& point2);

}  // namespace model
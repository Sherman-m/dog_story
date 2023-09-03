#include "geometry.h"

namespace model {

bool operator==(const Point& p1, const Point& p2) {
  return (std::fabs(p1.x - p2.x) <= std::numeric_limits<double>::epsilon()) &&
         (std::fabs(p1.y - p2.y) <= std::numeric_limits<double>::epsilon());
}

bool operator!=(const Point& p1, const Point& p2) { return !(p1 == p2); }

std::size_t PointHasher::operator()(const Point& point) const {
  const int first_prime_number = 137;
  const int second_prime_number = 149;
  return std::hash<int>{}(static_cast<int>(point.x) * first_prime_number +
                          static_cast<int>(point.y) * second_prime_number);
}

double PseudoscalarProductOfVectors(const Point& target_point,
                                    const Point& point1, const Point& point2) {
  return (point2.x - point1.x) * (target_point.y - point1.y) -
         (target_point.x - point1.x) * (point2.y - point1.y);
}

}  //  namespace model
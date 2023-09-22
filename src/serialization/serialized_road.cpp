#include "serialized_road.h"

namespace serialization {

SerializedRoad::SerializedRoad(const model::Road& road)
    : start_pos_(road.GetStartPosition()),
      end_pos_(road.GetEndPosition()),
      width_(road.GetWidth()) {}

model::Road SerializedRoad::Restore() const {
  double epsilon = 1e-10;
  if (std::fabs(start_pos_.y - end_pos_.y) < epsilon) {
    return model::Road(model::Road::HORIZONTAL, start_pos_, end_pos_.x);
  }
  return model::Road(model::Road::VERTICAL, start_pos_, end_pos_.y);
}

}  // namespace serialization
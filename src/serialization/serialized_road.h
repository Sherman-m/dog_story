#pragma once

#include "../../lib/model/road.h"
#include "serialized_geometry.h"

namespace serialization {

// Представляет сериализованный класс model::Road.
class SerializedRoad {
 public:
  SerializedRoad() = default;
  explicit SerializedRoad(const model::Road& road);

  model::Road Restore() const;

  template <typename Archive>
  void serialize(Archive& ar, [[maybe_unused]] const std::uint32_t version) {
    ar& start_pos_;
    ar& end_pos_;
    ar& width_;
  }

 private:
  model::Point start_pos_;
  model::Point end_pos_;
  model::Dimension width_;
};

}  // namespace serialization
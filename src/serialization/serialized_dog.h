#pragma once

#include "../../lib/model/dog.h"
#include "../../lib/model/geometry.h"
#include "serialized_geometry.h"
#include "serialized_lost_object.h"
#include "serialized_road.h"

namespace serialization {

// Представляет сериализованный класс model::Dog.
class SerializedDog {
 public:
  using SerializedBag = std::vector<SerializedLostObject>;

  SerializedDog() = default;
  explicit SerializedDog(const model::Dog& dog);

  model::Dog Restore() const;

  template <typename Archive>
  void serialize(Archive& ar, [[maybe_unused]] const std::uint32_t version) {
    ar&* id_;
    ar& name_;
    ar& width_;
    ar& curr_pos_;
    ar& prev_pos_;
    ar& speed_;
    ar& direction_;
    ar& curr_road_;
    ar& bag_capacity_;
    ar& bag_;
    ar& score_;
  }

 private:
  model::Dog::Id id_{0};
  std::string name_;
  model::Dimension width_;
  model::Point curr_pos_;
  model::Point prev_pos_;
  model::Speed speed_;
  model::Direction direction_;
  SerializedRoad curr_road_;
  std::uint32_t bag_capacity_;
  SerializedBag bag_;
  std::uint32_t score_;
};

}  // namespace serialization
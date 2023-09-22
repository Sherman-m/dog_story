#pragma once

#include "../../lib/model/lost_object.h"
#include "serialized_geometry.h"

namespace serialization {

// Представляет сериализованный класс model::LostObject.
class SerializedLostObject {
 public:
  SerializedLostObject() = default;
  explicit SerializedLostObject(const model::LostObject& lost_object);

  model::LostObject Restore() const;

  template <typename Archive>
  void serialize(Archive& ar, [[maybe_unused]] const std::uint32_t version) {
    ar&* id_;
    ar& type_;
    ar& value_;
    ar& pos_;
    ar& is_collected_;
  }

 private:
  model::LostObject::Id id_;
  std::uint32_t type_;
  std::uint32_t value_;
  model::Point pos_;
  bool is_collected_;
};

}  // namespace serialization
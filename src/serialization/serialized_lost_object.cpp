#include "serialized_lost_object.h"

namespace serialization {

SerializedLostObject::SerializedLostObject(const model::LostObject& lost_object)
    : id_(lost_object.GetId()),
      type_(lost_object.GetType()),
      value_(lost_object.GetValue()),
      pos_(lost_object.GetPosition()),
      is_collected_(lost_object.IsCollected()) {}

model::LostObject SerializedLostObject::Restore() const {
  model::LostObject lost_object(id_, type_, pos_, value_);
  if (is_collected_) {
    lost_object.Collect();
  }
  return lost_object;
}

}  // namespace serialization
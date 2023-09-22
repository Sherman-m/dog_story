#include "serialized_dog.h"

namespace serialization {

SerializedDog::SerializedDog(const model::Dog& dog)
    : id_(dog.GetId()),
      name_(dog.GetName()),
      width_(dog.GetWidth()),
      curr_pos_(dog.GetCurrentPosition()),
      prev_pos_(dog.GetPreviousPosition()),
      speed_(dog.GetSpeed()),
      direction_(dog.GetDirection()),
      curr_road_(dog.GetCurrentRoad()),
      bag_capacity_(dog.GetBagCapacity()),
      score_(dog.GetScore()) {
  bag_.reserve(bag_capacity_);
  for (auto lost_object : dog.GetBag()) {
    bag_.emplace_back(lost_object);
  }
}

model::Dog SerializedDog::Restore() const {
  model::Road dog_road(curr_road_.Restore());
  model::Dog dog(id_, name_, std::make_pair(curr_pos_, &dog_road),
                 bag_capacity_);
  dog.SetSpeed(speed_);
  dog.SetDirection(direction_);
  dog.AddScore(score_);
  for (auto& serialized_lost_object : bag_) {
    if (!dog.PutInBag(serialized_lost_object.Restore())) {
      throw std::runtime_error("Failed to put in dog's bag");
    }
  }
  return dog;
}

}  // namespace serialization
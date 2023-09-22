#include "serialized_game_session.h"

namespace serialization {

SerializedGameSession::SerializedGameSession(
    const model::GameSession& game_session)
    : id_(game_session.GetId()),
      name_(game_session.GetName()),
      map_id_(game_session.GetMapId()) {
  for (auto dog : game_session.GetDogs()) {
    dogs_.emplace_back(*dog);
  }
  for (auto& lost_object : game_session.GetLoot()) {
    loot_.emplace_back(lost_object);
  }
}

model::GameSession SerializedGameSession::Restore() const {
  model::GameSession game_session(id_, name_, map_id_);
  for (auto& dog : dogs_) {
    game_session.LoadDog(std::move(dog.Restore()));
  }
  for (auto& lost_object : loot_) {
    game_session.LoadLostObject(lost_object.Restore());
  }
  return game_session;
}

}  // namespace serialization
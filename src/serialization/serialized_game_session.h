#pragma once

#include "../../lib/model/game_session.h"
#include "../../lib/util/tagged.h"
#include "serialized_dog.h"
#include "serialized_geometry.h"
#include "serialized_lost_object.h"

namespace serialization {

// Представляет сериализованный класс model::GameSession.
class SerializedGameSession {
 public:
  using SerializedDogs = std::vector<SerializedDog>;
  using SerializedLoot = std::vector<SerializedLostObject>;

  SerializedGameSession() = default;
  explicit SerializedGameSession(const model::GameSession& game_session);

  model::GameSession Restore() const;

  template <typename Archive>
  void serialize(Archive& ar, [[maybe_unused]] const std::uint32_t version) {
    ar&* id_;
    ar& name_;
    ar&* map_id_;
    ar& dogs_;
    ar& loot_;
  }

 private:
  model::GameSession::Id id_;
  std::string name_;
  model::Map::Id map_id_;
  SerializedDogs dogs_;
  SerializedLoot loot_;
};

}  // namespace serialization
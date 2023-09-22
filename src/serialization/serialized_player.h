#pragma once

#include "../../lib/model/dog.h"
#include "../../lib/model/game_session.h"
#include "../app/player.h"

namespace serialization {

// Представляет сериализованный класс app::Player.
class SerializedPlayer {
 public:
  SerializedPlayer() = default;
  explicit SerializedPlayer(const app::Player &player);

  app::Player Restore() const;

  template <typename Archive>
  void serialize(Archive &ar, [[maybe_unused]] const std::uint32_t version) {
    ar &*id_;
    ar &*token_;
    ar &*game_session_id_;
    ar &*dog_id_;
  }

 private:
  app::Player::Id id_;
  app::Token token_;
  model::GameSession::Id game_session_id_;
  model::Dog::Id dog_id_;
};

}  // namespace serialization
#include "serialized_player.h"

namespace serialization {

SerializedPlayer::SerializedPlayer(const app::Player& player)
    : id_(player.GetId()),
      token_(player.GetToken()),
      game_session_id_(player.GetGameSessionId()),
      dog_id_(player.GetDogId()) {}

app::Player SerializedPlayer::Restore() const {
  return app::Player(id_, token_, game_session_id_, dog_id_);
}

}  // namespace serialization
#include "player.h"

namespace app {

Player::Player(Id id, Token token, model::GameSession::Id game_session_id,
               model::Dog::Id dog_id)
    : id_(id),
      token_(std::move(token)),
      game_session_id_(game_session_id),
      dog_id_(dog_id) {}

const Player::Id& Player::GetId() const noexcept { return id_; }

const Token& Player::GetToken() const noexcept { return token_; }

const model::GameSession::Id& Player::GetGameSessionId() const noexcept {
  return game_session_id_;
}

const model::Dog::Id& Player::GetDogId() const noexcept { return dog_id_; }

}  // namespace app
#pragma once

#include "../../lib/model/dog.h"
#include "../../lib/model/game_session.h"
#include "../../lib/util/tagged.h"
#include "token.h"

namespace app {

// Содержит данные об игроке.
class Player {
 public:
  using Id = util::Tagged<std::uint32_t, Player>;

  explicit Player(Id id, Token token, model::GameSession::Id game_session_id,
                  model::Dog::Id dog_id);

  const Id& GetId() const noexcept;
  const Token& GetToken() const noexcept;
  const model::GameSession::Id& GetGameSessionId() const noexcept;
  const model::Dog::Id& GetDogId() const noexcept;

 private:
  Id id_;
  Token token_;
  model::GameSession::Id game_session_id_;
  model::Dog::Id dog_id_;
};

}  // namespace app
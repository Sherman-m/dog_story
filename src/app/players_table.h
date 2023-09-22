#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../../lib/model/game_session.h"
#include "../../lib/model/retired_dog.h"
#include "../../lib/util/tagged.h"
#include "player.h"
#include "token.h"

namespace app {

using namespace std::literals;

// Содержит информацию обо всех игроках.
class PlayersTable {
 public:
  using Players = std::vector<const Player*>;

  PlayersTable() = default;
  PlayersTable(const PlayersTable&) = delete;
  PlayersTable& operator=(const PlayersTable&) = delete;

  Players GetPlayers() const;
  Players GetPlayersByGameSessionId(
      const model::GameSession::Id& game_session_id) const;
  const Player* GetPlayerByToken(const Token& token) const;
  const Player* GetPlayerByGameSessionIdAndDogId(
      const model::GameSession::Id& game_session_id,
      const model::Dog::Id& dog_id) const;
  const Player* AddPlayer(model::GameSession::Id game_session_id,
                          model::Dog::Id dog_id);
  // Добавляет уже сконструированного игрока, взятого из файла сохранения.
  const Player* LoadPlayer(Player player);

  void DeletePlayersByRetiredDogs(
      const model::GameSession::RetiredDogs& retired_dogs);

 private:
  using TokenToPlayer = std::unordered_map<Token, Player, TokenHasher>;

  TokenToPlayer token_to_player_;
  TokenGenerator token_generator_;
  std::uint32_t next_player_id_ = 0;
};

}  // namespace app

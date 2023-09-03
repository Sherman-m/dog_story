#include "players_table.h"

namespace app {

PlayersTable::Players PlayersTable::GetPlayers() const {
  Players players;
  for (const auto& player : token_to_player_) {
    players.push_back(&player.second);
  }
  return players;
}

PlayersTable::Players PlayersTable::GetPlayersByGameSessionId(
    const model::GameSession::Id& game_session_id) const {
  Players players;
  for (const auto& player : token_to_player_) {
    if (player.second.GetGameSessionId() == game_session_id) {
      players.push_back(&player.second);
    }
  }
  return players;
}

Player* PlayersTable::GetPlayerByToken(const Token& token) noexcept {
  if (auto it = token_to_player_.find(token); it != token_to_player_.end()) {
    return &(it->second);
  }
  return nullptr;
}

const Player* PlayersTable::GetPlayerByToken(
    const Token& token) const noexcept {
  if (auto it = token_to_player_.find(token); it != token_to_player_.end()) {
    return &(it->second);
  }
  return nullptr;
}

std::pair<Token, const Player*> PlayersTable::AddPlayer(
    model::GameSession::Id game_session_id, model::Dog::Id dog_id) {
  Player player(Player::Id(++next_player_id_), game_session_id, dog_id);
  Token token = token_generator_.Generate();
  int attempts = 5;
  while (token_to_player_.contains(token)) {
    token = token_generator_.Generate();
    if (--attempts == 0) {
      throw std::runtime_error("Failed to generate unique token"s);
    }
  }
  try {
    auto [it, inserted] = token_to_player_.emplace(std::move(token), player);
    return std::make_pair(it->first, &(it->second));
  } catch (...) {
    --next_player_id_;
    throw;
  }
}

}  // namespace app
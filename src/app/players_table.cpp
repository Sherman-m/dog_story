#include "players_table.h"

namespace app {

PlayersTable::Players PlayersTable::GetPlayers() const {
  Players players;
  players.reserve(token_to_player_.size());
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

const Player* PlayersTable::GetPlayerByToken(const Token& token) const {
  if (auto it = token_to_player_.find(token); it != token_to_player_.end()) {
    return &(it->second);
  }
  return nullptr;
}

const Player* PlayersTable::GetPlayerByGameSessionIdAndDogId(
    const model::GameSession::Id& game_session_id,
    const model::Dog::Id& dog_id) const {
  for (const auto& player : token_to_player_) {
    if (player.second.GetDogId() == dog_id &&
        player.second.GetGameSessionId() == game_session_id) {
      return &player.second;
    }
  }
  return nullptr;
}

const Player* PlayersTable::AddPlayer(model::GameSession::Id game_session_id,
                                      model::Dog::Id dog_id) {
  Token token = token_generator_.Generate();
  int attempts = 5;
  while (token_to_player_.contains(token)) {
    token = token_generator_.Generate();
    if (--attempts == 0) {
      throw std::runtime_error("Failed to generate unique token"s);
    }
  }
  Player player(Player::Id(++next_player_id_), token, game_session_id, dog_id);
  try {
    auto [it, inserted] =
        token_to_player_.emplace(std::move(token), std::move(player));
    return &it->second;
  } catch (...) {
    --next_player_id_;
    throw std::runtime_error("Failed to add player with id == "s +
                             std::to_string(next_player_id_ + 1));
  }
}

// Так как игрок уже был сконструирован ранее, то у него уже имеется свой id.
// Для того чтобы id игроков были уникальными, нужно при добавлении игрока
// изменить next_player_id_.
const Player* PlayersTable::LoadPlayer(Player player) {
  Token token(player.GetToken());
  Player::Id player_id(player.GetId());
  std::uint32_t temp_text_player_id_ = next_player_id_;
  try {
    auto [it, inserted] =
        token_to_player_.emplace(std::move(token), std::move(player));
    next_player_id_ = std::max(next_player_id_, *it->second.GetId());
    return &it->second;
  } catch (...) {
    next_player_id_ = temp_text_player_id_;
    throw std::runtime_error("Failed to load player with id == "s +
                             std::to_string(*player_id));
  }
}

void PlayersTable::DeletePlayersByRetiredDogs(
    const model::GameSession::RetiredDogs& retired_dogs) {
  for (const auto& dog : retired_dogs) {
    if (auto player = GetPlayerByGameSessionIdAndDogId(dog.GetGameSessionId(),
                                                       dog.GetId())) {
      token_to_player_.erase(player->GetToken());
    }
  }
}

}  // namespace app
#include "game.h"

namespace model {

Game::Game(LootGenerator loot_generator)
    : loot_generator_(std::move(loot_generator)) {}

const Game::Maps& Game::GetMaps() const noexcept { return maps_; }

void Game::AddMap(Map map) {
  if (map_id_to_index_.contains(map.GetId())) {
    throw std::invalid_argument("Map with id "s + *map.GetId() +
                                " already exists"s);
  }
  const size_t index = maps_.size();
  Map& m = maps_.emplace_back(std::move(map));
  try {
    map_id_to_index_.emplace(m.GetId(), index);
  } catch (...) {
    maps_.pop_back();
    throw;
  }
}

const Map* Game::GetMapById(const Map::Id& id) const noexcept {
  if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
    return &maps_.at(it->second);
  }
  return nullptr;
}

Game::GameSessions Game::GetGameSessions() {
  GameSessions game_sessions;
  for (auto& game_session : game_session_id_to_game_session) {
    game_sessions.push_back(&game_session.second);
  }
  return game_sessions;
}

GameSession* Game::GetGameSessionById(
    const GameSession::Id& game_session_id) noexcept {
  if (auto it = game_session_id_to_game_session.find(game_session_id);
      it != game_session_id_to_game_session.end()) {
    return &(it->second);
  }
  return nullptr;
}

GameSession* Game::GetGameSessionByMapId(const Map::Id& map_id) noexcept {
  for (auto& game_session : game_session_id_to_game_session) {
    if (game_session.second.GetMapId() == map_id) {
      return &game_session.second;
    }
  }
  return nullptr;
}

GameSession* Game::AddGameSession(std::string game_session_name,
                                  Map::Id map_id) {
  GameSession game_session(GameSession::Id(++next_game_session_id_),
                           std::move(game_session_name), std::move(map_id));
  try {
    auto [it, inserted] = game_session_id_to_game_session.emplace(
        game_session.GetId(), std::move(game_session));
    return &it->second;
  } catch (...) {
    --next_game_session_id_;
    throw;
  }
}

// Для каждой игровой сессии вычисляет число потерянных вещей, которые нужно
// добавить в игровую сессию, и передает эту информацию в
// game_session->UpdateSession.
void Game::UpdateGameSession(const GameSession::Id& game_session_id,
                             Game::Milliseconds time_delta) {
  using namespace std::chrono;
  if (auto game_session = GetGameSessionById(game_session_id)) {
    auto map = GetMapById(game_session->GetMapId());
    auto dogs = game_session->GetDogs();
    std::uint32_t loot_count =
        loot_generator_.Generate(time_delta, game_session->GetLoot().size(),
                                 game_session->GetDogs().size());
    game_session->UpdateSession(map, loot_count, time_delta);
  } else {
    throw std::invalid_argument("Game session with id"s +
                                std::to_string(*game_session_id) +
                                "does not exist"s);
  }
}

Dog* Game::AddDogInGameSession(const GameSession::Id& game_session_id,
                               std::string dog_name,
                               const std::pair<Point, const Road*>& dog_pos) {
  if (auto game_session = GetGameSessionById(game_session_id)) {
    auto map = GetMapById(game_session->GetMapId());
    return game_session->AddDog(std::move(dog_name), dog_pos,
                                map->GetBagCapacity());
  }
  throw std::invalid_argument("Game session with id "s +
                              std::to_string(*game_session_id) +
                              "does not exist"s);
}

void Game::MoveDog(const GameSession::Id& game_session_id,
                   const Dog::Id& dog_id, const std::string& movement) {
  if (auto game_session = GetGameSessionById(game_session_id)) {
    if (auto map = GetMapById(game_session->GetMapId())) {
      auto dog_speed_on_map = map->GetDogSpeed();
      game_session->MoveDog(dog_id, dog_speed_on_map, movement);
    } else {
      throw std::invalid_argument("Map with id "s + *game_session->GetMapId() +
                                  "does not exist"s);
    }
  } else {
    throw std::invalid_argument("Game session with id "s +
                                std::to_string(*game_session_id) +
                                "does not exist"s);
  }
}

}  // namespace model
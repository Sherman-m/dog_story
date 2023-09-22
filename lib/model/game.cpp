#include "game.h"

namespace model {

Game::Game(LootGenerator loot_generator)
    : loot_generator_(std::move(loot_generator)) {}

const Game::Maps& Game::GetMaps() const noexcept { return maps_; }

const Map* Game::GetMapById(const Map::Id& id) const {
  if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
    return &maps_.at(it->second);
  }
  return nullptr;
}

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
    throw std::runtime_error("Failed to add map with id == "s + *map.GetId());
  }
}

Game::GameSessions Game::GetGameSessions() {
  GameSessions game_sessions;
  game_sessions.reserve(game_session_id_to_game_session.size());
  for (auto& game_session : game_session_id_to_game_session) {
    game_sessions.push_back(&game_session.second);
  }
  return game_sessions;
}

Game::ConstGameSessions Game::GetGameSessions() const {
  ConstGameSessions game_sessions;
  game_sessions.reserve(game_session_id_to_game_session.size());
  for (auto& game_session : game_session_id_to_game_session) {
    game_sessions.push_back(&game_session.second);
  }
  return game_sessions;
}

GameSession* Game::GetGameSessionById(const GameSession::Id& game_session_id) {
  if (auto it = game_session_id_to_game_session.find(game_session_id);
      it != game_session_id_to_game_session.end()) {
    return &(it->second);
  }
  return nullptr;
}

const GameSession* Game::GetGameSessionById(
    const GameSession::Id& game_session_id) const {
  if (auto it = game_session_id_to_game_session.find(game_session_id);
      it != game_session_id_to_game_session.end()) {
    return &(it->second);
  }
  return nullptr;
}

GameSession* Game::GetFirstGameSessionByMapId(const Map::Id& map_id) {
  for (auto& game_session : game_session_id_to_game_session) {
    if (game_session.second.GetMapId() == map_id) {
      return &game_session.second;
    }
  }
  return nullptr;
}

const GameSession* Game::GetFirstGameSessionByMapId(
    const Map::Id& map_id) const {
  for (auto& game_session : game_session_id_to_game_session) {
    if (game_session.second.GetMapId() == map_id) {
      return &game_session.second;
    }
  }
  return nullptr;
}

Game::GameSessions Game::GetAllGameSessionsByMapId(const Map::Id& map_id) {
  GameSessions game_sessions;
  for (auto& game_session : game_session_id_to_game_session) {
    if (game_session.second.GetMapId() == map_id) {
      game_sessions.push_back(&game_session.second);
    }
  }
  return game_sessions;
}

Game::ConstGameSessions Game::GetAllGameSessionsByMapId(
    const Map::Id& map_id) const {
  ConstGameSessions game_sessions;
  for (auto& game_session : game_session_id_to_game_session) {
    if (game_session.second.GetMapId() == map_id) {
      game_sessions.push_back(&game_session.second);
    }
  }
  return game_sessions;
}

GameSession* Game::AddGameSession(std::string game_session_name,
                                  Map::Id map_id) {
  using namespace std::literals;

  if (auto map = GetMapById(map_id); !map) {
    throw std::invalid_argument("Map with id "s + *map_id + " already exists"s);
  }
  GameSession game_session(GameSession::Id(++next_game_session_id_),
                           std::move(game_session_name), std::move(map_id));
  try {
    auto [it, inserted] = game_session_id_to_game_session.emplace(
        game_session.GetId(), std::move(game_session));
    return &it->second;
  } catch (...) {
    --next_game_session_id_;
    throw std::runtime_error("Failed to add game session with id == "s +
                             std::to_string(next_game_session_id_ + 1));
  }
}

// Так как игровая сессия уже была сконструирована ранее, то у нее уже имеется
// свой id. Для того чтобы id игровых сессий были уникальными, нужно при
// добавлении игровой сессии изменить next_game_session_id_.
GameSession* Game::LoadGameSession(model::GameSession game_session) {
  using namespace std::literals;

  if (auto map = GetMapById(game_session.GetMapId()); !map) {
    throw std::invalid_argument("Map with id "s + *game_session.GetMapId() +
                                " already exists"s);
  }
  std::uint32_t temp_next_game_session_id = next_game_session_id_;
  GameSession::Id game_session_id(game_session.GetId());
  try {
    auto [it, inserted] = game_session_id_to_game_session.emplace(
        game_session_id, std::move(game_session));
    next_game_session_id_ = std::max(next_game_session_id_, *it->first);
    return &it->second;
  } catch (...) {
    next_game_session_id_ = temp_next_game_session_id;
    throw std::runtime_error("Failed to load game session with id == "s +
                             std::to_string(*game_session_id));
  }
}

// Для каждой игровой сессии вычисляет число потерянных вещей, которые нужно
// добавить в игровую сессию, и передает эту информацию в
// game_session->UpdateSession.
GameSession::RetiredDogs Game::UpdateGameSession(
    const GameSession::Id& game_session_id, Game::Milliseconds time_delta) {
  using namespace std::chrono;
  if (auto game_session = GetGameSessionById(game_session_id)) {
    auto map = GetMapById(game_session->GetMapId());
    std::uint32_t loot_count = loot_generator_.Generate(
        time_delta, game_session->GetLootCount(), game_session->GetDogsCount());
    auto retired_dogs =
        game_session->UpdateSession(map, loot_count, time_delta);
    if (game_session->IsEmpty()) {
      game_session_id_to_game_session.erase(game_session_id);
    }
    return retired_dogs;
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
    auto map = GetMapById(game_session->GetMapId());
    auto dog_speed_on_map = map->GetDogSpeed();
    game_session->MoveDog(dog_id, dog_speed_on_map, movement);

  } else {
    throw std::invalid_argument("Game session with id "s +
                                std::to_string(*game_session_id) +
                                "does not exist"s);
  }
}

}  // namespace model
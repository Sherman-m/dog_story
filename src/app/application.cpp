#include "application.h"

#include <utility>

namespace app {

const model::Game::Maps& Application::GetMaps() const noexcept {
  return game_.GetMaps();
}

const model::Map* Application::GetMapById(const model::Map::Id& map_id) const {
  using namespace std::literals;
  try {
    return game_.GetMapById(map_id);
  } catch (const std::exception& ec) {
    LogError(ec.what(), "Getting map by id"sv);
    return nullptr;
  }
}

PlayersTable::Players Application::GetAllPlayers() const {
  try {
    return players_table_.GetPlayers();
  } catch (const std::exception& ec) {
    LogError(ec.what(), "Getting all players"sv);
    return {};
  }
}

PlayersTable::Players Application::GetPlayersByGameSessionId(
    const model::GameSession::Id& game_session_id) const {
  try {
    return players_table_.GetPlayersByGameSessionId(game_session_id);
  } catch (const std::exception& ec) {
    LogError(ec.what(), "Getting all players in game session with id == "s +
                            std::to_string(*game_session_id));
    return {};
  }
}

const Player* Application::GetPlayerByToken(const Token& token_player) const {
  try {
    return players_table_.GetPlayerByToken(token_player);
  } catch (const std::exception& ec) {
    LogError(ec.what(), "Getting player by token"sv);
    return nullptr;
  }
}

const Player* Application::GetPlayerByMapIdAndDogName(
    const model::Map::Id& map_id, const std::string& dog_name) const {
  try {
    for (auto game_session : game_.GetAllGameSessionsByMapId(map_id)) {
      if (auto dog = game_session->GetDogByName(dog_name)) {
        return players_table_.GetPlayerByGameSessionIdAndDogId(
            game_session->GetId(), dog->GetId());
      }
    }
    return nullptr;
  } catch (const std::exception& ec) {
    LogError(ec.what(), "Getting all player in game session with map id == "s +
                            *map_id + " and dog name == "s + dog_name);
    return nullptr;
  }
}

bool Application::MovePlayer(const model::GameSession::Id& game_session_id,
                             const model::Dog::Id& dog_id,
                             const std::string& movement) {
  bool result = false;
  if (auto game_session_strand = strand_storage_.GetStrand(game_session_id)) {
    auto handler = [self = this->shared_from_this(), &game_session_id, &dog_id,
                    &movement, &result] {
      try {
        self->game_.MoveDog(game_session_id, dog_id, movement);
        result = true;
      } catch (const std::exception& ec) {
        self->LogError(ec.what(), "Moving player in game session with id == "s +
                                      std::to_string(*game_session_id) +
                                      " and with dog id == "s +
                                      std::to_string(*dog_id));
        result = false;
      }
    };
    net::dispatch(*game_session_strand, std::move(handler));
  }
  return result;
}

model::GameSession* Application::GetGameSessionById(
    const model::GameSession::Id& game_session_id) {
  try {
    return game_.GetGameSessionById(game_session_id);
  } catch (const std::exception& ec) {
    LogError(ec.what(), "Getting game session by id == "s +
                            std::to_string(*game_session_id));
    return nullptr;
  }
}

const model::GameSession* Application::GetGameSessionById(
    const model::GameSession::Id& game_session_id) const {
  try {
    return game_.GetGameSessionById(game_session_id);
  } catch (const std::exception& ec) {
    LogError(ec.what(), "Getting game session by id == "s +
                            std::to_string(*game_session_id));
    return nullptr;
  }
}

model::GameSession* Application::GetGameSessionByMapId(
    const model::Map::Id& map_id) {
  try {
    return game_.GetFirstGameSessionByMapId(map_id);
  } catch (const std::exception& ec) {
    LogError(ec.what(), "Getting game session by map id == "s + *map_id);
    return nullptr;
  }
}

const model::GameSession* Application::GetGameSessionByMapId(
    const model::Map::Id& map_id) const {
  try {
    return game_.GetFirstGameSessionByMapId(map_id);
  } catch (const std::exception& ec) {
    LogError(ec.what(), "Getting game session by map id == "s + *map_id);
    return nullptr;
  }
}

// Создает новую игровую сессию.
model::GameSession* Application::CreateGameSession(
    model::Map::Id map_id, std::string game_session_name) {
  model::GameSession* result = nullptr;
  try {
    result =
        game_.AddGameSession(std::move(game_session_name), std::move(map_id));
    strand_storage_.AddStrand(result->GetId());
  } catch (const std::exception& ec) {
    LogError(ec.what(), "Creating game session with map id == "s + *map_id +
                            " and game session name == "s + game_session_name);
    result = nullptr;
  }
  return result;
}

const Player* Application::JoinToGameSession(
    std::string dog_name, model::GameSession::Id game_session_id,
    const std::pair<model::Point, const model::Road*>& dog_position) {
  const Player* result = nullptr;
  if (auto game_session_strand = strand_storage_.GetStrand(game_session_id)) {
    auto handler = [self = this->shared_from_this(), &dog_name,
                    &game_session_id, &dog_position, &result] {
      try {
        auto dog = self->game_.AddDogInGameSession(
            game_session_id, std::move(dog_name), dog_position);
        result = self->players_table_.AddPlayer(game_session_id, dog->GetId());
      } catch (const std::exception& ec) {
        self->LogError(ec.what(), "Joining to game session with id == "s +
                                      std::to_string(*game_session_id));
        result = nullptr;
      }
    };
    net::dispatch(*game_session_strand, std::move(handler));
  }
  return result;
}

bool Application::UpdateGameSession(
    const model::GameSession::Id& game_session_id, Milliseconds time_delta) {
  bool result = false;
  if (auto game_session_strand = strand_storage_.GetStrand(game_session_id)) {
    auto handler = [self = this->shared_from_this(), &game_session_id,
                    &time_delta, &result] {
      try {
        auto retired_dogs =
            self->game_.UpdateGameSession(game_session_id, time_delta);
        if (!retired_dogs.empty()) {
          self->players_table_.DeletePlayersByRetiredDogs(retired_dogs);
          self->SaveRetiredPlayers(retired_dogs);
        }
        result = true;
      } catch (const std::exception& ec) {
        self->LogError(ec.what(), "Updating game session with id == "s +
                                      std::to_string(*game_session_id));
        result = false;
      }
    };
    net::dispatch(*game_session_strand, std::move(handler));
  }
  return result;
}

bool Application::UpdateAllGameSessions(Milliseconds time_delta) {
  try {
    for (auto game_session : game_.GetGameSessions()) {
      if (!UpdateGameSession(game_session->GetId(), time_delta)) {
        return false;
      }
    }
    return true;
  } catch (const std::exception& ec) {
    LogError(ec.what(), "Updating all game sessions"sv);
    return false;
  }
}

bool Application::SaveGameState() const {
  if (!is_save_file_set_) {
    return true;
  }
  using namespace serialization;
  const std::string_view where = "Saving the game state to a file"sv;

  std::vector<SerializedGameSession> serialized_game_sessions;
  std::ofstream out(kTempSaveFile);
  if (!out.is_open()) {
    LogError("Failed to open a temporary file to save the game state"sv, where);
    return false;
  }
  try {
    boost::archive::text_oarchive ar{out, std::ios_base::binary};
    for (auto game_session : game_.GetGameSessions()) {
      serialized_game_sessions.emplace_back(*game_session);
    }
    ar << serialized_game_sessions;
    std::vector<SerializedPlayer> serialized_players;
    for (auto player : players_table_.GetPlayers()) {
      serialized_players.emplace_back(*player);
    }
    ar << serialized_players;
    fs::rename(kTempSaveFile, kSaveFile);
    return true;
  } catch (const std::exception& ec) {
    LogError(ec.what(), where);
    return false;
  }
}

void Application::LoadGameState() {
  using namespace serialization;
  const std::string_view where = "Loading the saved game state from a file"sv;

  std::ifstream in(kSaveFile);
  if (!in.is_open()) {
    return LogError("Failed to open the saved game state file"sv, where);
  }
  try {
    boost::archive::text_iarchive ar{in, std::ios_base::binary};
    std::vector<SerializedGameSession> serialized_game_sessions;
    ar >> serialized_game_sessions;
    for (auto& serialized_game_session : serialized_game_sessions) {
      auto game_session =
          game_.LoadGameSession(std::move(serialized_game_session.Restore()));
      strand_storage_.AddStrand(game_session->GetId());
    }
    std::vector<SerializedPlayer> serialized_players;
    ar >> serialized_players;
    for (auto& serialized_player : serialized_players) {
      players_table_.LoadPlayer(std::move(serialized_player.Restore()));
    }
  } catch (const std::exception& ec) {
    LogError(ec.what(), where);
    throw;
  }
}

void Application::SaveRetiredPlayers(
    const model::GameSession::RetiredDogs& retired_dogs) {
  auto unit_of_work = database_.CreateUnitOfWork();
  unit_of_work.GetRetiredPlayersRepository().SaveRetiredPlayers(retired_dogs);
  unit_of_work.Commit();
}

model::GameSession::RetiredDogs Application::GetRetiredPlayers(
    std::uint32_t offset, std::uint32_t max_items) {
  try {
    auto unit_of_work = database_.CreateUnitOfWork();
    auto result = unit_of_work.GetRetiredPlayersRepository().GetRetiredPlayers(
        offset, max_items);
    unit_of_work.Commit();
    return result;
  } catch (...) {
    LogError("Failed to get retired players"sv, "Getting retired players"sv);
    return {};
  }
}

void Application::LogError(std::string_view text_error,
                           std::string_view where) const {
  logger::Log(json::value{{"text"s, text_error}, {"where"s, where}}, "error"sv);
}

}  // namespace app
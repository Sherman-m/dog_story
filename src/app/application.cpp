#include "application.h"

namespace app {

Application::Application(net::io_context& ioc, model::Game& game,
                         Milliseconds tick_period, bool ticker_is_set)
    : strand_storage_(ioc),
      game_(game),
      tick_period_(tick_period),
      ticker_is_set_(ticker_is_set) {}

const model::Game::Maps& Application::GetMaps() const noexcept {
  return game_.GetMaps();
}

const model::Map* Application::GetMapById(
    const model::Map::Id& map_id) const noexcept {
  return game_.GetMapById(map_id);
}

PlayersTable::Players Application::GetAllPlayers() const {
  return players_table_.GetPlayers();
}

PlayersTable::Players Application::GetPlayersInSession(
    const model::GameSession::Id& game_session_id) const {
  return players_table_.GetPlayersByGameSessionId(game_session_id);
}

const Player* Application::GetPlayerByToken(
    const Token& token_player) const noexcept {
  return players_table_.GetPlayerByToken(token_player);
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
      } catch (const std::invalid_argument&) {
        result = false;
      }
    };
    net::dispatch(*game_session_strand, std::move(handler));
  }
  return result;
}

model::GameSession* Application::GetGameSessionById(
    const model::GameSession::Id& game_session_id) noexcept {
  return game_.GetGameSessionById(game_session_id);
}

model::GameSession* Application::GetGameSessionByMapId(
    const model::Map::Id& map_id) noexcept {
  return game_.GetGameSessionByMapId(map_id);
}

// Создает новую игровую сессию. Если ticker_is_set_ == true, то для этой
// игровой сессии создается собственный Ticker, который будет обновлять игровое
// состояние сессии через каждые ticker_period_.
model::GameSession* Application::CreateGameSession(
    model::Map::Id map_id, std::string game_session_name) {
  model::GameSession* result = nullptr;
  try {
    result =
        game_.AddGameSession(std::move(game_session_name), std::move(map_id));
    auto game_session_strand = strand_storage_.AddStrand(result->GetId());
    if (ticker_is_set_) {
      auto ticker = std::make_shared<Ticker>(
          result->GetId(), *game_session_strand, tick_period_,
          [self = this->shared_from_this()](
              const model::GameSession::Id& game_session_id,
              Milliseconds time_delta) -> bool {
            return self->UpdateGameSession(game_session_id, time_delta);
          });
      ticker->Start();
    }
  } catch (...) {
    result = nullptr;
  }
  return result;
}

std::pair<Token, const Player*> Application::JoinToGameSession(
    std::string dog_name, model::GameSession::Id game_session_id,
    const std::pair<model::Point, const model::Road*>& dog_position) {
  std::pair<Token, const Player*> result = std::make_pair(Token(""s), nullptr);
  if (auto game_session_strand = strand_storage_.GetStrand(game_session_id)) {
    auto handler = [self = this->shared_from_this(), &dog_name,
                    &game_session_id, &dog_position, &result] {
      try {
        auto dog = self->game_.AddDogInGameSession(
            game_session_id, std::move(dog_name), dog_position);
        result = self->players_table_.AddPlayer(game_session_id, dog->GetId());
      } catch (...) {
        result = std::make_pair(Token(""s), nullptr);
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
        self->game_.UpdateGameSession(game_session_id, time_delta);
        result = true;
      } catch (const std::invalid_argument&) {
        result = false;
      }
    };
    net::dispatch(*game_session_strand, std::move(handler));
  }
  return result;
}

bool Application::UpdateAllGameSessions(Milliseconds time_delta) {
  for (auto game_session : game_.GetGameSessions()) {
    if (!UpdateGameSession(game_session->GetId(), time_delta)) {
      return false;
    }
  }
  return true;
}

bool Application::IsTickerSet() const noexcept { return ticker_is_set_; }

}  // namespace app
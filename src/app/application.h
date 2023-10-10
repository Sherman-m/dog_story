#pragma once

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/json.hpp>
#include <boost/serialization/vector.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <utility>
#include <vector>

#include "../../lib/model/game.h"
#include "../../lib/model/game_session.h"
#include "../../lib/model/map.h"
#include "../db/database.h"
#include "../logger/logger.h"
#include "../serialization/serialization.h"
#include "player.h"
#include "players_table.h"
#include "strand_storage.h"
#include "token.h"

namespace app {

namespace net = boost::asio;
namespace json = boost::json;
namespace fs = std::filesystem;

// Реализует паттерн "Facade" и предоставляет user cases для работы с модулями
// app и model.
//
// Функции, взаимодействующие с игровыми сессиями, выполняются с помощью
// последовательного исполнителя. Это позволяет избежать гонки данных.
//
// Также этот класс отвечает за создание последовательных исполнителей для
// каждой игровой сессии (при создании этих игровых сессий), чтобы игровые
// сессии могли изменять свое состояние параллельно.
//
// В некоторых функциях-членах используются сырые указатели на GameSession, Map,
// Dog и Player, так как мы не продлеваем жизнь объекта, а только предоставляем
// доступ для взаимодействия с объектом.
class Application : public std::enable_shared_from_this<Application> {
 public:
  using Milliseconds = std::chrono::milliseconds;

  template <typename ConnectionFactory>
  explicit Application(net::io_context& ioc, model::Game& game,
                       std::string save_file, bool is_save_file_set,
                       const db::DatabaseConfig<ConnectionFactory> config)
      : strand_storage_(ioc),
        game_(game),
        kSaveFile(std::move(save_file)),
        is_save_file_set_(is_save_file_set),
        database_(config) {
    if (is_save_file_set_ && fs::exists(kSaveFile)) {
      LoadGameState();
    }
  }
  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

  const model::Game::Maps& GetMaps() const noexcept;

  // Возвращает указатель на карту с id равном map_id.
  // При неудаче возвращает пустой std::vector<const Player*>.
  const model::Map* GetMapById(const model::Map::Id& map_id) const;

  // Возвращает std::vector<const Player*> во всех игровых сессиях.
  // При неудаче возвращает пустой std::vector<const Player*>.
  PlayersTable::Players GetAllPlayers() const;

  // Возвращает std::vector<const Player*> в игровой сессии с id равном
  // game_session_id.
  // При неудаче возвращает пустой std::vector<const Player*>.
  PlayersTable::Players GetPlayersByGameSessionId(
      const model::GameSession::Id& game_session_id) const;

  // Возвращает указатель на игрока, токен которого равен token_player.
  // При неудаче возвращает nullptr.
  const Player* GetPlayerByToken(const Token& token_player) const;

  const Player* GetPlayerByMapIdAndDogName(const model::Map::Id& map_id,
                                           const std::string& dog_name) const;

  // Меняет направление собаки с id равно dog_id в игровой сессии с id равном
  // game_session_id на movement.
  bool MovePlayer(const model::GameSession::Id& game_session_id,
                  const model::Dog::Id& dog_id, const std::string& movement);

  // Возвращает указатель на игровую сессию с id равном game_session_id.
  // При неудаче возвращает nullptr.
  model::GameSession* GetGameSessionById(
      const model::GameSession::Id& game_session_id);
  const model::GameSession* GetGameSessionById(
      const model::GameSession::Id& game_session_id) const;

  // Возвращает указатель на первую попавшеюся игровую сессию с id равном
  // map_id.
  // При неудаче возвращает nullptr.
  model::GameSession* GetGameSessionByMapId(const model::Map::Id& map_id);
  const model::GameSession* GetGameSessionByMapId(
      const model::Map::Id& map_id) const;

  // Создает новую игровую сессию.
  // При неудаче возвращает nullptr.
  model::GameSession* CreateGameSession(
      model::Map::Id map_id, std::string game_session_name = "new session"s);

  // Добавляет нового игрока в таблицу players_table_ и связанную с ним собаку в
  // игровую сессию с id равном game_session_id.
  // При неудаче возвращает nullptr.
  const Player* JoinToGameSession(
      std::string dog_name, model::GameSession::Id game_session_id,
      const std::pair<model::Point, const model::Road*>& dog_position);

  // Обновляет игровую сессию с id равном game_session_id внутри своего strand.
  // При неудаче возвращает false.
  bool UpdateGameSession(const model::GameSession::Id& game_session_id,
                         Milliseconds time_delta);

  // Вызывает внутри себя UpdateGameSession для всех игровых сессий.
  bool UpdateAllGameSessions(Milliseconds time_delta);

  // Сохраняет состояние игры в kSaveFile.
  bool SaveGameState() const;

  // Восстанавливает состояние игры их kSaveFile.
  void LoadGameState();

  // Сохранение "уставших" игроков в базу данных.
  void SaveRetiredPlayers(const model::GameSession::RetiredDogs& retired_dogs);

  // Загрузка "уставших" игроков из базы данных.
  model::GameSession::RetiredDogs GetRetiredPlayers(std::uint32_t offset,
                                                    std::uint32_t max_items);

 private:
  void LogError(std::string_view error_text, std::string_view where) const;

  StrandStorage strand_storage_;
  model::Game& game_;
  PlayersTable players_table_;
  const std::string kSaveFile;
  const std::string kTempSaveFile = kSaveFile + "temp_";
  bool is_save_file_set_;
  db::Database database_;
};

}  // namespace app
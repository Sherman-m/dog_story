#pragma once

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../util/tagged.h"
#include "game_session.h"
#include "loot_generator.h"
#include "lost_object.h"
#include "map.h"
#include "retired_dog.h"

namespace model {

// Описывает объект игры. Является основой игры. Содержит всю информацию об
// игровых объектах и игровых сессиях.
//
// Изменение игровых сессий и генерация потерянных объектов выполняются
// с помощью объекта Game.
//
// В некоторых функциях-членах используются сырые указатели на GameSession, Map
// и Dog, так как мы не продлеваем жизнь объекта, а только предоставляем доступ
// для взаимодействия с объектом.
class Game {
 public:
  using Maps = std::vector<Map>;
  using GameSessions = std::vector<GameSession*>;
  using ConstGameSessions = std::vector<const GameSession*>;
  using Milliseconds = std::chrono::milliseconds;

  explicit Game(LootGenerator loot_generator);

  const Maps& GetMaps() const noexcept;

  // Ищет игровую карту по ее id и возвращает указатель на эту карту.
  // Если карты с таким id не существует, то возвращает nullptr.
  const Map* GetMapById(const Map::Id& id) const;

  void AddMap(Map map);

  GameSessions GetGameSessions();
  ConstGameSessions GetGameSessions() const;

  // Ищет игровую сессию по ее id и возвращает указатель на эту игровую сессию.
  // Если игровой сессии с таким id не существует, то возвращает nullptr;
  GameSession* GetGameSessionById(const GameSession::Id& game_session_id);
  const GameSession* GetGameSessionById(
      const GameSession::Id& game_session_id) const;

  // Находит первую попавшеюся игровую сессию, id карты которой равно map_id, и
  // возвращает указатель на эту игровую сессию.
  // В случае неудачи возвращает nullptr.
  GameSession* GetFirstGameSessionByMapId(const Map::Id& map_id);
  const GameSession* GetFirstGameSessionByMapId(const Map::Id& map_id) const;

  GameSessions GetAllGameSessionsByMapId(const Map::Id& map_id);
  ConstGameSessions GetAllGameSessionsByMapId(const Map::Id& map_id) const;

  // Конструирует новую игровую сессию в game_session_id_to_game_session и
  // возвращает указатель на этот сконструированный объект.
  GameSession* AddGameSession(std::string game_session_name, Map::Id map_id);

  // Добавляет уже сконструированную игровую сессию, взятую из файла сохранения.
  GameSession* LoadGameSession(GameSession game_session);

  GameSession::RetiredDogs UpdateGameSession(
      const GameSession::Id& game_session_id, Milliseconds time_delta);

  Dog* AddDogInGameSession(const GameSession::Id& game_session_id,
                           std::string dog_name,
                           const std::pair<Point, const Road*>& dog_pos);

  void MoveDog(const GameSession::Id& game_session_id, const Dog::Id& dog_id,
               const std::string& movement);

 private:
  using MapIdHasher = util::TaggedHasher<Map::Id>;
  using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
  using GameSessionIdHasher = util::TaggedHasher<GameSession::Id>;
  using GameSessionIdToGameSession =
      std::unordered_map<GameSession::Id, GameSession, GameSessionIdHasher>;

  Maps maps_;
  MapIdToIndex map_id_to_index_;
  GameSessionIdToGameSession game_session_id_to_game_session;
  std::uint32_t next_game_session_id_ = 0;
  LootGenerator loot_generator_;
};

}  // namespace model
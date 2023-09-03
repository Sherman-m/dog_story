#pragma once

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
  using Milliseconds = std::chrono::milliseconds;

  explicit Game(LootGenerator loot_generator);

  const Maps& GetMaps() const noexcept;

  const Map* GetMapById(const Map::Id& id) const noexcept;

  void AddMap(Map map);

  GameSessions GetGameSessions();

  GameSession* GetGameSessionById(
      const GameSession::Id& game_session_id) noexcept;

  GameSession* GetGameSessionByMapId(const Map::Id& map_id) noexcept;

  GameSession* AddGameSession(std::string game_session_name, Map::Id map_id);

  void UpdateGameSession(const GameSession::Id& game_session_id,
                         Milliseconds time_delta);

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
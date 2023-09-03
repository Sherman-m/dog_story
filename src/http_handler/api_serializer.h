#pragma once

#include <boost/json.hpp>
#include <string>

#include "../../lib/json_loader/loot_types_storage.h"
#include "../../lib/model/model.h"
#include "../app/application.h"

namespace http_handler {

namespace json = boost::json;

using namespace std::literals;

// Отвечает за сериализацию ответа в JSON-формата.
struct ApiSerializer {
  ApiSerializer() = delete;
  ApiSerializer(ApiSerializer&) = delete;
  ApiSerializer& operator=(ApiSerializer&) = delete;
  ApiSerializer(ApiSerializer&&) = delete;
  ApiSerializer& operator=(ApiSerializer&&) = delete;

  // GetJson(Roads|Buildings|Offices|DogBag|Loot|Players) отвечают за приведение
  // массива объектов (Roads|Buildings|Offices) к json::object или json::array
  static json::array GetJsonRoads(const model::Map::Roads& roads);
  static json::array GetJsonBuildings(const model::Map::Buildings& buildings);
  static json::array GetJsonOffices(const model::Map::Offices& offices);
  static json::array GetJsonDogBag(const model::Dog::Bag& dog_bag);
  static json::object GetJsonLoot(const model::GameSession::Loot& loot);
  static json::object GetJsonPlayers(const model::GameSession* game_session,
                                     const app::PlayersTable::Players& players);

  // Serialized(Map|Maps|Player|Players|JoinResponse|Error) отвечают за создание
  // сериализованных данных для последующей отправки клиенту
  static std::string SerializeMap(const model::Map* map);
  static std::string SerializeMaps(const model::Game::Maps& maps);
  static std::string SerializePlayer(const app::Player* player);
  static std::string SerializePlayersInGameSession(
      const model::GameSession* game_session,
      const app::PlayersTable::Players& players);
  static std::string SerializeState(const model::GameSession* game_session,
                                    const app::PlayersTable::Players& players);
  static std::string SerializeJoinResponse(
      const std::pair<app::Token, const app::Player*>& join_response);
  static std::string SerializeError(std::string_view code,
                                    std::string_view message);
};
}  // namespace http_handler
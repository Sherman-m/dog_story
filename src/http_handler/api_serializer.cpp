#include "api_serializer.h"

namespace http_handler {

json::array ApiSerializer::GetJsonRoads(const model::Map::Roads& roads) {
  json::array roads_info;
  for (const auto& road : roads) {
    json::object road_info;
    road_info["x0"s] = road.GetStartPosition().x;
    road_info["y0"s] = road.GetStartPosition().y;
    if (road.IsHorizontal()) {
      road_info["x1"s] = road.GetEndPosition().x;
    } else {
      road_info["y1"s] = road.GetEndPosition().y;
    }
    roads_info.push_back(road_info);
  }
  return roads_info;
}

json::array ApiSerializer::GetJsonBuildings(
    const model::Map::Buildings& buildings) {
  json::array buildings_info;
  for (const auto& building : buildings) {
    json::object building_info;
    building_info["x"s] = building.GetBounds().pos.x;
    building_info["y"s] = building.GetBounds().pos.y;
    building_info["w"s] = building.GetBounds().size.width;
    building_info["h"s] = building.GetBounds().size.height;
    buildings_info.push_back(building_info);
  }
  return buildings_info;
}

json::array ApiSerializer::GetJsonOffices(const model::Map::Offices& offices) {
  json::array offices_info;
  for (const auto& office : offices) {
    json::object office_info;
    office_info["id"s] = *office.GetId();
    office_info["x"s] = office.GetPosition().x;
    office_info["y"s] = office.GetPosition().y;
    office_info["offsetX"s] = office.GetOffset().dx;
    office_info["offsetY"s] = office.GetOffset().dy;
    offices_info.push_back(office_info);
  }
  return offices_info;
}

json::array ApiSerializer::GetJsonDogBag(const model::Dog::Bag& dog_bag) {
  json::array dog_bag_info;
  for (auto& lost_object : dog_bag) {
    json::object lost_object_info;
    lost_object_info["id"s] = *lost_object.GetId();
    lost_object_info["type"s] = lost_object.GetType();
    dog_bag_info.push_back(lost_object_info);
  }
  return dog_bag_info;
}

json::object ApiSerializer::GetJsonLoot(const model::GameSession::Loot& loot) {
  json::object loot_info;
  for (const auto& lost_object : loot) {
    json::object lost_object_info;
    model::Point lost_object_position = lost_object.GetPosition();
    lost_object_info["type"s] = lost_object.GetType();
    lost_object_info["pos"s] =
        json::array{lost_object_position.x, lost_object_position.y};

    auto lost_object_id = std::to_string(*lost_object.GetId());
    loot_info[lost_object_id] = lost_object_info;
  }
  return loot_info;
}

json::object ApiSerializer::GetJsonPlayers(
    const model::GameSession* game_session,
    const app::PlayersTable::Players& players) {
  json::object players_info;
  for (const auto& player : players) {
    const auto player_dog = game_session->GetDogById(player->GetDogId());
    model::Point dog_position = player_dog->GetCurrentPosition();
    model::Speed dog_speed = player_dog->GetSpeed();
    model::Direction dog_direction = player_dog->GetDirection();

    json::object player_dog_info;
    player_dog_info["pos"s] = json::array{dog_position.x, dog_position.y};
    player_dog_info["speed"s] = json::array{dog_speed.sx, dog_speed.sy};
    player_dog_info["dir"s] = std::string{static_cast<char>(dog_direction)};

    json::array dog_bag_info = GetJsonDogBag(player_dog->GetBag());
    player_dog_info["bag"s] = dog_bag_info;
    player_dog_info["score"s] = player_dog->GetScore();
    auto player_id = std::to_string(*player->GetId());
    players_info[player_id] = player_dog_info;
  }
  return players_info;
}

std::string ApiSerializer::SerializeMap(const model::Map* map) {
  json::object map_info;
  map_info["id"s] = *map->GetId();
  map_info["name"s] = map->GetName();
  map_info["roads"s] = GetJsonRoads(map->GetRoads());
  map_info["buildings"s] = GetJsonBuildings(map->GetBuildings());
  map_info["offices"s] = GetJsonOffices(map->GetOffices());
  auto loot_types =
      json_loader::LootTypesStorage::GetTypesOfLoots(map->GetId());
  if (loot_types) {
    map_info["lootTypes"s] = *loot_types;
  }
  return json::serialize(map_info);
}

std::string ApiSerializer::SerializeMaps(const model::Game::Maps& maps) {
  json::array maps_info;
  for (const auto& map : maps) {
    json::object map_info;
    map_info["id"s] = *map.GetId();
    map_info["name"s] = map.GetName();
    maps_info.push_back(map_info);
  }
  return json::serialize(maps_info);
}

std::string ApiSerializer::SerializePlayer(const app::Player* player) {
  json::object player_info;
  player_info["id"s] = *player->GetId();
  player_info["game_session_id"s] = *player->GetGameSessionId();
  player_info["dog_id"s] = *player->GetDogId();
  return json::serialize(player_info);
}

std::string ApiSerializer::SerializePlayersInGameSession(
    const model::GameSession* game_session,
    const app::PlayersTable::Players& players) {
  json::object players_info;
  for (const auto& player : players) {
    json::object player_info;
    player_info["name"s] =
        game_session->GetDogById(player->GetDogId())->GetName();

    auto player_id = std::to_string(*player->GetId());
    players_info[player_id] = player_info;
  }
  return json::serialize(players_info);
}

std::string ApiSerializer::SerializeState(
    const model::GameSession* game_session,
    const app::PlayersTable::Players& players) {
  json::object game_session_info;

  json::object players_info = GetJsonPlayers(game_session, players);
  game_session_info["players"s] = players_info;

  json::object loot_info = GetJsonLoot(game_session->GetLoot());
  game_session_info["lostObjects"s] = loot_info;
  return json::serialize(game_session_info);
}

std::string ApiSerializer::SerializeJoinResponse(const app::Player* player) {
  json::object join_response_info;
  join_response_info["authToken"s] = *player->GetToken();
  join_response_info["playerId"s] = *player->GetId();
  return json::serialize(join_response_info);
}

std::string ApiSerializer::SerializeRecordsResponse(
    const model::GameSession::RetiredDogs& retired_dogs) {
  json::array records_info;
  for (auto& dog : retired_dogs) {
    json::object retired_dog_info;
    retired_dog_info["name"s] = dog.GetName();
    retired_dog_info["score"s] = dog.GetScore();
    retired_dog_info["playTime"s] =
        std::chrono::duration_cast<Seconds>(dog.GetPlayTime()).count();
    records_info.push_back(retired_dog_info);
  }
  return json::serialize(records_info);
}

std::string ApiSerializer::SerializeError(std::string_view code,
                                          std::string_view message) {
  json::object error_info;
  error_info["code"s] = std::string(code);
  error_info["message"s] = std::string(message);
  return json::serialize(error_info);
}

}  // namespace http_handler
#pragma once

#include <boost/json.hpp>
#include <unordered_map>

#include "../model/map.h"
#include "../util/tagged.h"

namespace json_loader {

namespace json = boost::json;

// Хранилище для типов потерянных объектов. Содержит специфичную информацию о
// потерянных объектов, необходимую для фронтенда (для отрисовки объектов и
// тд.). Представляет собой словарь "<id_карты>": "<JSON-массив данных об
// объектах на карте>".
class LootTypesStorage {
 public:
  using LootTypes = std::unordered_map<model::Map::Id, json::array,
                                       util::TaggedHasher<model::Map::Id>>;

  LootTypesStorage() = delete;
  LootTypesStorage(const LootTypesStorage&) = delete;
  LootTypesStorage& operator=(const LootTypesStorage&) = delete;
  LootTypesStorage(LootTypesStorage&&) = delete;
  LootTypesStorage& operator=(LootTypesStorage&&) = delete;

  // Возвращает сырой указатель на JSON-массив данных о типе потерянного
  // объекта. Если такого типа не существует, возвращает nullptr
  static const json::array* GetTypesOfLoots(
      const model::Map::Id& map_id) noexcept;

  static void AddTypesOfLoots(const model::Map::Id& map_id,
                              const boost::json::array& loot_types);

 private:
  static LootTypes loot_types_;
};

}  // namespace json_loader
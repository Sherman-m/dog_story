#pragma once

#include <boost/json.hpp>

#include "../model/model.h"
#include "../util/file_handler.h"
#include "loot_types_storage.h"

namespace json_loader {

namespace json = boost::json;

using namespace std::literals;

// Приводит json::value к std::string.
std::string JsonObjectToString(const json::value& value);

// Получает описание дороги в формате JSON, исходя из этих данных создает объект
// model::Road и возвращает этот объект.
model::Road DeserializeRoad(const json::value& json_road);

// Получает описание здания в формате JSON, исходя из этих данных создает объект
// model::Building и возвращает этот объект.
model::Building DeserializeBuilding(const json::value& json_building);

// Получает описание офиса в формате JSON, исходя из этих данных создает объект
// model::Office и возвращает этот объект.
model::Office DeserializeOffice(const json::value& json_office);

// Получает описание игровой карты в формате JSON и данные, нужные для
// конструирования объекта model::Map, исходя из этих данных создает объект
// model::Map и возвращает этот объект.
model::Map DeserializeMap(const json::object& json_map,
                          model::Speed& default_dog_speed,
                          std::uint32_t num_of_loot_types,
                          std::uint32_t default_bag_capacity);

// Получает путь к конфигурационному файлу config.json, содержащий информацию
// об объектах игры. Создает хранилище для типов потерянных объектов
// LootTypesStorage, конструирует объект model::Game, заполняет его информацией
// о картах и возвращает его.
model::Game LoadGame(const std::filesystem::path& json_path);

}  // namespace json_loader
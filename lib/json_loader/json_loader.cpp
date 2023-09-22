#include "json_loader.h"

namespace json_loader {

std::string JsonObjectToString(const json::value& value) {
  if (!value.is_string()) {
    throw std::invalid_argument("Value is not a string"s);
  }
  return json::value_to<std::string>(value);
}

model::Road DeserializeRoad(const json::value& json_road) {
  model::Point start_point(json_road.at("x0"s).to_number<model::Coord>(),
                           json_road.at("y0"s).to_number<model::Coord>());
  if (auto end_x = json_road.as_object().if_contains("x1"s)) {
    return model::Road(model::Road::HORIZONTAL, start_point,
                       end_x->to_number<model::Coord>());
  }
  auto end_y = json_road.as_object().if_contains("y1"s);
  return model::Road(model::Road::VERTICAL, start_point,
                     end_y->to_number<model::Coord>());
}

model::Building DeserializeBuilding(const json::value& json_building) {
  model::Point building_point(json_building.at("x"s).to_number<model::Coord>(),
                              json_building.at("y"s).to_number<model::Coord>());
  model::Size building_size(
      json_building.at("w"s).to_number<model::Dimension>(),
      json_building.at("h"s).to_number<model::Dimension>());
  model::Rectangle rectangle(building_point, building_size);
  return model::Building(rectangle);
}

model::Office DeserializeOffice(const json::value& json_office) {
  model::Office::Id office_id(JsonObjectToString(json_office.at("id"s)));
  model::Point office_point(json_office.at("x"s).to_number<model::Coord>(),
                            json_office.at("y"s).to_number<model::Coord>());
  model::Offset office_offset(
      json_office.at("offsetX"s).to_number<model::Coord>(),
      json_office.at("offsetY"s).to_number<model::Coord>());
  return model::Office(office_id, office_point, office_offset);
}

// Вызывает внутри себя функции:
//  - DeserializeRoad(const json::value&);
//  - DeserializeBuilding(const json::value&);
//  - DeserializeOffice(const json::value&);
//  - JsonObjectToString(const json::value& value);
model::Map DeserializeMap(const json::object& json_map,
                          model::Speed& default_dog_speed,
                          std::uint32_t num_of_loot_types,
                          std::uint32_t default_bag_capacity,
                          Seconds dog_retirement_time) {
  if (const auto& dog_speed = json_map.if_contains("dogSpeed"s)) {
    default_dog_speed =
        model::Speed(static_cast<float>(dog_speed->as_double()),
                     static_cast<float>(dog_speed->as_double()));
  }
  if (const auto& bag_capacity = json_map.if_contains("bagCapacity"s)) {
    default_bag_capacity =
        static_cast<std::uint32_t>(bag_capacity->as_uint64());
  }
  model::Map::Id map_id(JsonObjectToString(json_map.at("id"s)));
  model::Map map(map_id, JsonObjectToString(json_map.at("name"s)),
                 default_dog_speed, num_of_loot_types, default_bag_capacity,
                 dog_retirement_time);
  for (const auto& map_road : json_map.at("roads"s).as_array()) {
    try {
      model::Road road = DeserializeRoad(map_road);
      map.AddRoad(road);
    } catch (const std::exception& e) {
      throw std::runtime_error("Error when deserializing the road"s + e.what());
    }
  }
  for (const auto& map_building : json_map.at("buildings"s).as_array()) {
    try {
      model::Building building = DeserializeBuilding(map_building);
      map.AddBuilding(building);
    } catch (const std::exception& e) {
      throw std::runtime_error("Error when deserializing the building"s +
                               e.what());
    }
  }
  for (const auto& map_office : json_map.at("offices"s).as_array()) {
    try {
      model::Office office = DeserializeOffice(map_office);
      map.AddOffice(std::move(office));
    } catch (const std::exception& e) {
      throw std::runtime_error("Error when deserializing the office"s +
                               e.what());
    }
  }
  return map;
}

// Вызывает внутри себя функции:
//  - DeserializeRoad(const json::value&);
//  - DeserializeBuilding(const json::value&);
//  - DeserializeOffice(const json::value&);
//  - DeserializeMap(const json::object& json_map,
//                   model::Speed& default_dog_speed,
//                   std::uint32_t num_of_loot_types,
//                   std::uint32_t default_bag_capacity);
//  - JsonObjectToString(const json::value& value);
model::Game LoadGame(const std::filesystem::path& json_path) {
  std::string json_obj_to_str = util::LoadContentFromFile(json_path);

  auto json_content = json::parse(json_obj_to_str).as_object();
  auto loot_generator_config =
      json_content.at("lootGeneratorConfig"s).as_object();

  model::LootGenerator loot_generator(
      Seconds(
          static_cast<long>(loot_generator_config.at("period"s).as_double())),
      loot_generator_config.at("probability"s).as_double());

  model::Game game(std::move(loot_generator));
  Seconds default_dog_retirement_time(60);
  if (const auto& dog_retirement_time =
          json_content.if_contains("dogRetirementTime"s)) {
    default_dog_retirement_time =
        Seconds(static_cast<long>(dog_retirement_time->as_double()));
  }
  for (const auto& json_map : json_content.at("maps"s).as_array()) {
    model::Speed default_dog_speed(1.0, 1.0);
    if (const auto& dog_speed = json_content.if_contains("defaultDogSpeed"s)) {
      default_dog_speed =
          model::Speed(dog_speed->as_double(), dog_speed->as_double());
    }
    try {
      auto loot_types = json_map.at("lootTypes"s).as_array();
      std::uint32_t default_bag_capacity = 3;
      if (const auto& bag_capacity =
              json_content.if_contains("defaultBagCapacity"s)) {
        default_bag_capacity =
            static_cast<std::uint32_t>(bag_capacity->as_uint64());
      }
      model::Map map = DeserializeMap(json_map.as_object(), default_dog_speed,
                                      loot_types.size(), default_bag_capacity,
                                      default_dog_retirement_time);
      LootTypesStorage::AddTypesOfLoots(map.GetId(), loot_types);
      game.AddMap(std::move(map));
    } catch (const std::exception& e) {
      throw std::runtime_error("Error during parsing config.json: "s +
                               e.what());
    }
  }
  return game;
}

}  // namespace json_loader

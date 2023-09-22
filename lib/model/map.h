#pragma once

#include <chrono>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../util/tagged.h"
#include "building.h"
#include "geometry.h"
#include "office.h"
#include "road.h"

namespace model {
// Описывает карту. Содержит информацию об дорогах, зданиях и офисах.
class Map {
 public:
  using Id = util::Tagged<std::string, Map>;
  using Roads = std::vector<Road>;
  using Buildings = std::vector<Building>;
  using Offices = std::vector<Office>;
  using Milliseconds = std::chrono::milliseconds;

  Map(Id id, std::string name, const Speed& dog_speed,
      std::uint32_t num_of_loot_types, std::uint32_t bag_capacity,
      Milliseconds dog_retirement_time);

  const Id& GetId() const noexcept;

  const std::string& GetName() const noexcept;

  const Roads& GetRoads() const noexcept;

  const Buildings& GetBuildings() const noexcept;

  const Offices& GetOffices() const noexcept;

  Speed GetDogSpeed() const noexcept;

  Milliseconds GetDogRetirementTime() const noexcept;

  std::uint32_t GetBagCapacity() const noexcept;

  // Находит дорогу, на которую может перейти собака, исходя из ее координат
  // начала и конца движения.
  std::pair<Point, const Road*> GetRoadFromTo(const Point& from,
                                              const Point& to) const;

  void AddRoad(const Road& target_road);

  void AddBuilding(const Building& building);

  void AddOffice(Office office);

  // Генерирует рандомную позицию с помощью генератора псевдослучайных чисел.
  // randomize_spawn_position == false - возвращает начало первой дороги на
  //                                     карте.
  std::pair<Point, const Road*> GenerateRandomPosition(
      bool randomize_spawn_position = true) const;

  // Генерирует рандомный тип потерянной вещи с помощью генератора
  // псевдослучайных чисел.
  std::uint32_t GenerateRandomLootType() const;

 private:
  using OfficeIdHasher = util::TaggedHasher<Office::Id>;
  using OfficeIdToIndex =
      std::unordered_map<Office::Id, size_t, OfficeIdHasher>;
  using RoadIntersectionPoints =
      std::unordered_map<Point, std::unordered_set<Road, RoadHasher>,
                         PointHasher>;

  // Проверяет, что одна дорога совпадает (является частью) другой.
  void CheckRoadMatching(Coord target_road_start_coord,
                         Coord target_road_end_coord, const Road& target_road,
                         Coord road_on_map_start_coord,
                         Coord road_on_map_end_coord, const Road& road_on_map);

  Id id_;
  std::string name_;
  Speed dog_speed_{1.0, 1.0};
  Milliseconds dog_retirement_time_;
  Roads roads_;
  RoadIntersectionPoints road_intersection_points_;
  Buildings buildings_;
  OfficeIdToIndex warehouse_id_to_index_;
  Offices offices_;
  std::uint32_t num_of_loot_types_ = 1;
  std::uint32_t bag_capacity_;

  // Генератор псевдослучайных чисел, который будет использоваться в
  // GenerateRandomPosition и GenerateRandomLootType
  mutable std::mt19937 random_engine_{std::random_device{}()};
};

}  // namespace model
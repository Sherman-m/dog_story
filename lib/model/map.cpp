#include "map.h"

namespace model {

Map::Map(Map::Id id, std::string name, const Speed& dog_speed,
         std::uint32_t num_of_loot_types, std::uint32_t bag_capacity,
         Milliseconds dog_retirement_time)
    : id_(std::move(id)),
      name_(std::move(name)),
      dog_speed_(dog_speed),
      num_of_loot_types_(num_of_loot_types),
      bag_capacity_(bag_capacity),
      dog_retirement_time_(dog_retirement_time) {}

const Map::Id& Map::GetId() const noexcept { return id_; };

const std::string& Map::GetName() const noexcept { return name_; }

const Map::Roads& Map::GetRoads() const noexcept { return roads_; }

const Map::Buildings& Map::GetBuildings() const noexcept { return buildings_; }

const Map::Offices& Map::GetOffices() const noexcept { return offices_; }

Speed Map::GetDogSpeed() const noexcept { return dog_speed_; }

Map::Milliseconds Map::GetDogRetirementTime() const noexcept {
  return dog_retirement_time_;
}

std::uint32_t Map::GetBagCapacity() const noexcept { return bag_capacity_; }

// Собака может менять дорогу только в том случае, когда ее стартовая точка
// находится вблизи точки пересечения.
//
// Алгоритм функции следующий:
//  1. Округляем стартовую точку from, чтобы получить потенциальную точку
//     пересечения;
//  2. Проверяем, если такая дорога, в пределах которой лежит точка назначения
//     to:
//   2.1 Если такая дорога есть, возвращаем точку и указатель на эту дорогу.
//  3. Если дорога не нашлась, тогда рассмотрим сценарий, когда при переходе на
//     другую дорогу мы выходим за ее границу (такое может случиться при
//     тестировании движения собаки через обращение к /api/v1/game/tick):
//   3.1 Если такая дорога есть, возвращаем указатель на точку, обозначающую
//       границу этой дороги, и указатель на эту дорогу.
//  4. Если никакая дорога не нашлась, возвращаем любую точку и nullptr,
//     обозначающий, что дорога не найдена.
std::pair<Point, const Road*> Map::GetRoadFromTo(const Point& from,
                                                 const Point& to) const {
  auto intersection_point = Point(std::round(from.x), std::round(from.y));
  if (auto const& it = road_intersection_points_.find(intersection_point);
      it != road_intersection_points_.end()) {
    // Проверяем, есть ли такая дорога, в пределах которой лежит точка to
    for (auto const& road : it->second) {
      if (road.IsInsideRoad(to)) {
        // возвращаем точку и дорогу
        return std::make_pair(to, &road);
      }
    }
    for (auto const& road : it->second) {
      auto road_start = road.GetStartPosition();
      auto road_end = road.GetEndPosition();
      auto half_road_width = road.GetWidth() / 2;

      if (std::fabs(from.x - to.x) <= std::numeric_limits<double>::epsilon() &&
          road.IsVertical() &&
          (to.x >= road_end.x - half_road_width &&
           to.x <= road_end.x + half_road_width)) {
        double py = (to.y > from.y) ? std::max(road_start.y, road_end.y)
                                    : std::min(road_start.y, road_end.y);
        return std::make_pair(Point(to.x, py), &road);
      } else if (std::fabs(from.y - to.y) <=
                     std::numeric_limits<double>::epsilon() &&
                 road.IsHorizontal() &&
                 (to.y >= road_end.y - half_road_width &&
                  to.y <= road_end.y + half_road_width)) {
        double px = (to.x > from.x) ? std::max(road_start.x, road_end.x)
                                    : std::min(road_start.x, road_end.x);
        return std::make_pair(Point(px, to.y), &road);
      }
    }
  }
  return std::make_pair(Point(0, 0), nullptr);
}

// Находит точки пересечения дорог, и заполняет словарь вида:
// <точка пересечения>: std::set<Road>.
//
// В основе расчетов лежит решение системы канонических уравнений
// прямой вида:
//  1. (x - x1)/(x2 - x1) = (y - y1)/(y2 - y1) = t1;
//  2. (x - x3)/(x4 - x3) = (y - y3)/(y4 - y3) = t2.
// Алгоритм функции следующий:
//  1. Находим denominator для уравнений:
//     t1 = ((x3 - x1) * (y4 - y3) + (y1 - y3) * (x4 - x3)) / denominator;
//     t2 = ((y1 - y3) * (x2 - x1) + (x3 - x1) * (y2 - y1)) / denominator;
//     denominator = (x2 - x1) * (y4 - y3) - (y2 - y1) * (x4 - x3).
//  2. Если denominator == 0, то проверяем, совпадают ли прямые с помощью
//     функции CheckRoadMatching;
//  3. Если denominator != 0, то находим числители для каждого уравнения:
//     numerator_t1 = (x3 - x1) * (y4 - y3) + (y1 - y3) * (x4 - x3);
//     numerator_t2 = (y1 - y3) * (x2 - x1) + (x3 - x1) * (y2 - y1);
//   3.1 Находим t1 и t2. Если они лежат в интервале [0, 1], то прямые
//       пересекаются (t1 и t2 характеризуют пересечение проекций на оси X и Y);
//   3.2 Находим точку пересечения и добавляем связки этой точки с дорогами
//       target_road и road_on_map в словарь road_intersection_points_.
//  4. Добавляем связки точки начала и конца дороги target_road с самой дорогой
//     target_road в словарь road_intersection_points_.
void Map::AddRoad(const Road& target_road) {
  Point target_road_start = target_road.GetStartPosition();
  Point target_road_end = target_road.GetEndPosition();
  for (const auto& road_on_map : roads_) {
    Point road_on_map_start = road_on_map.GetStartPosition();
    Point road_on_map_end = road_on_map.GetEndPosition();

    double denominator = (target_road_end.x - target_road_start.x) *
                             (road_on_map_end.y - road_on_map_start.y) -
                         (target_road_end.y - target_road_start.y) *
                             (road_on_map_end.x - road_on_map_start.x);

    if (denominator == 0) {
      if (target_road.IsHorizontal() && road_on_map.IsHorizontal() &&
          static_cast<int>(target_road_start.y) ==
              static_cast<int>(road_on_map_start.y)) {
        CheckRoadMatching(target_road_start.x, target_road_end.x, target_road,
                          road_on_map_start.x, road_on_map_end.x, road_on_map);
      } else if (target_road.IsVertical() && road_on_map.IsVertical() &&
                 static_cast<int>(target_road_start.x) ==
                     static_cast<int>(road_on_map_start.x)) {
        CheckRoadMatching(target_road_start.y, target_road_end.y, target_road,
                          road_on_map_start.y, road_on_map_end.y, road_on_map);
      }
    } else {
      double numerator_t1 = (road_on_map_start.x - target_road_start.x) *
                                (road_on_map_end.y - road_on_map_start.y) +
                            (target_road_start.y - road_on_map_start.y) *
                                (road_on_map_end.x - road_on_map_start.x);
      double numerator_t2 = (target_road_start.y - road_on_map_start.y) *
                                (target_road_end.x - target_road_start.x) +
                            (road_on_map_start.x - target_road_start.x) *
                                (target_road_end.y - target_road_start.y);

      double t1 = numerator_t1 / denominator;
      double t2 = numerator_t2 / denominator;

      if (t1 >= 0 && t1 <= 1 && t2 >= 0 && t2 <= 1) {
        auto intersection_point =
            Point(target_road_start.x +
                      t1 * (target_road_end.x - target_road_start.x),
                  target_road_start.y +
                      t1 * (target_road_end.y - target_road_start.y));
        road_intersection_points_[intersection_point].insert(target_road);
        road_intersection_points_[intersection_point].insert(road_on_map);
      }
    }
  }
  road_intersection_points_[target_road_start].insert(target_road);
  road_intersection_points_[target_road_end].insert(target_road);
  roads_.emplace_back(target_road);
}

void Map::AddBuilding(const Building& building) {
  buildings_.emplace_back(building);
}

void Map::AddOffice(Office office) {
  if (warehouse_id_to_index_.contains(office.GetId())) {
    throw std::invalid_argument("Duplicate warehouse"s);
  }

  const size_t index = offices_.size();
  Office& o = offices_.emplace_back(std::move(office));
  try {
    warehouse_id_to_index_.emplace(o.GetId(), index);
  } catch (...) {
    offices_.pop_back();
    throw;
  }
}

std::pair<Point, const Road*> Map::GenerateRandomPosition(
    bool randomize_spawn_position) const {
  if (randomize_spawn_position) {
    std::uniform_int_distribution<std::size_t> road_index_distrib(
        0, roads_.size() - 1);

    std::size_t road_index = road_index_distrib(random_engine_);
    model::Point start_road = roads_[road_index].GetStartPosition();
    model::Point end_road = roads_[road_index].GetEndPosition();

    std::uniform_real_distribution<model::Coord> x_coord_distrib(start_road.x,
                                                                 end_road.x);
    std::uniform_real_distribution<model::Coord> y_coord_distrib(start_road.y,
                                                                 end_road.y);
    return std::make_pair(model::Point(x_coord_distrib(random_engine_),
                                       y_coord_distrib(random_engine_)),
                          &roads_[road_index]);
  }
  return std::make_pair(roads_[0].GetStartPosition(), &roads_[0]);
}

std::uint32_t Map::GenerateRandomLootType() const {
  std::uniform_int_distribution<std::uint32_t> type_distrib(
      0, num_of_loot_types_ - 1);
  return type_distrib(random_engine_);
}

// Проверяет, лежат ли точки конца и начала дороги в пределах другой дороги.
// Если лежат, то эти точки связываются с дорогой, в пределах которой они лежат,
// и добавляются в словарь road_intersection_points_.
void Map::CheckRoadMatching(Coord target_road_start_coord,
                            Coord target_road_end_coord,
                            const Road& target_road,
                            Coord road_on_map_start_coord,
                            Coord road_on_map_end_coord,
                            const Road& road_on_map) {
  if (target_road_start_coord >= road_on_map_start_coord &&
      target_road_start_coord <= road_on_map_end_coord) {
    road_intersection_points_[target_road.GetStartPosition()].insert(
        road_on_map);
  }
  if (target_road_end_coord >= road_on_map_start_coord &&
      target_road_end_coord <= road_on_map_end_coord) {
    road_intersection_points_[target_road.GetEndPosition()].insert(road_on_map);
  }
  if (road_on_map_start_coord >= target_road_start_coord &&
      road_on_map_start_coord <= target_road_end_coord) {
    road_intersection_points_[road_on_map.GetStartPosition()].insert(
        target_road);
  }
  if (road_on_map_end_coord >= target_road_start_coord &&
      road_on_map_end_coord <= target_road_end_coord) {
    road_intersection_points_[road_on_map.GetEndPosition()].insert(target_road);
  }
}

}  // namespace model
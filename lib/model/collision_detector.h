#pragma once

#include <algorithm>
#include <cmath>
#include <list>
#include <memory>
#include <vector>

#include "dog.h"
#include "geometry.h"
#include "lost_object.h"

namespace model {

// Описывает результат столкновения двух объектов.
// Используется в качестве возвращаемого значения в функции TryCollectPoint.
struct CollisionResult {
  // В качестве входных данных получает радиус столкновения двух объектов.
  // Возвращает результат проверки двух событий:
  //  - попадает ли доля пройденного отрезка в интервал [0, 1];
  //  - квадрат расстояния до точки меньше квадрата радиуса столкновения двух
  //    объектов.
  bool IsCollected(double collect_radius) const;

  // Квадрат расстояния до точки.
  double sq_distance;

  // Доля пройденного отрезка.
  double proj_ratio;
};

// Получает координаты начала и конца движения игрока и координату объекта,
// с которым может столкнуться игрок. Возвращает результат столкновения.
// Работает корректно только при ненулевом перемещении.
CollisionResult TryCollectPoint(Point dog_pos_start, Point dog_pos_end,
                                Point object_pos);

// Описывает тип события столкновения. Есть два типа столкновения:
//  - kCollect - столкновение с model::LostObject
//  - kPass - столкновение с model::Office
enum class CollisionEventType : char { kCollect = 'C', kPass = 'P' };

// Описывает событие столкновения.
struct CollisionEvent {
  using LostObjectPosition = LostObject::Loot::iterator;
  // Создает объект CollisionEvent.
  //
  // Если тип столкновения равен kPass, то lost_object_ptr должен быть равен
  // nullptr, а lost_object_pos - концу GameSession::Loot.
  //
  // Если тип столкновения равен kCollect, то lost_object_ptr должен указывать
  // на новый объект LostObject, созданный в куче на основе объекта,
  // хранящегося в GameSession::Loot, а lost_object_pos - на позицию объекта
  // LostObject, хранящегося в GameSession::Loot.
  // Это нужно для того, чтобы при добавлении объекта LostObject в рюкзак игрока
  // можно было удалять оригинал этого объекта из GameSession::Loot, но в это же
  // время иметь доступ к данным этого объекта.
  explicit CollisionEvent(CollisionEventType type, const Dog::Id& dog_id,
                          const std::shared_ptr<LostObject>& lost_object_ptr,
                          LostObjectPosition lost_object_pos,
                          double sq_distance, double time);

  CollisionEventType type;
  Dog::Id dog_id;
  std::shared_ptr<LostObject> lost_object_ptr;
  LostObjectPosition lost_object_pos;
  double sq_distance;
  double time;
};

bool operator==(const CollisionEvent& event1, const CollisionEvent& event2);

bool operator!=(const CollisionEvent& event1, const CollisionEvent& event2);

bool operator<(const CollisionEvent& event1, const CollisionEvent& event2);

bool operator>(const CollisionEvent& event1, const CollisionEvent& event2);

}  // namespace model
#include "collision_detector.h"

namespace model {

bool CollisionResult::IsCollected(double collect_radius) const {
  return proj_ratio >= 0 && proj_ratio <= 1 &&
         sq_distance <= collect_radius * collect_radius;
}

CollisionEvent::CollisionEvent(
    CollisionEventType type, const Dog::Id& dog_id,
    const std::shared_ptr<LostObject>& lost_object_ptr,
    LostObjectPosition lost_object_pos, double sq_distance, double time)
    : type(type),
      dog_id(dog_id),
      lost_object_ptr(lost_object_ptr),
      lost_object_pos(lost_object_pos),
      sq_distance(sq_distance),
      time(time) {}

bool operator==(const CollisionEvent& event1, const CollisionEvent& event2) {
  const double epsilon = 1e-10;
  return event1.type == event2.type && event1.dog_id == event2.dog_id &&
         event1.lost_object_ptr == event2.lost_object_ptr &&
         event1.lost_object_pos == event2.lost_object_pos &&
         std::fabs(event1.sq_distance - event2.sq_distance) <= epsilon &&
         std::fabs(event1.time - event2.time) <= epsilon;
}

bool operator!=(const CollisionEvent& event1, const CollisionEvent& event2) {
  return !(event1 == event2);
}

bool operator<(const CollisionEvent& event1, const CollisionEvent& event2) {
  return event1.time < event2.time;
}

bool operator>(const CollisionEvent& event1, const CollisionEvent& event2) {
  return event2 < event1;
}

// Вычисляет векторы u и v, где u - вектор перемещения собаки, v - вектор,
// указывающий на объект, с которым может столкнуться игрок. Считает скалярное
// произведение и длины этих векторов. Находит долю пройденного отрезка до
// столкновения с объектом и квадрат расстояния до объекта.
CollisionResult TryCollectPoint(Point dog_pos_start, Point dog_pos_end,
                                Point object_pos) {
  const double u_x = object_pos.x - dog_pos_start.x;
  const double u_y = object_pos.y - dog_pos_start.y;
  const double v_x = dog_pos_end.x - dog_pos_start.x;
  const double v_y = dog_pos_end.y - dog_pos_start.y;
  const double u_dot_v = u_x * v_x + u_y * v_y;
  const double u_len2 = u_x * u_x + u_y * u_y;
  const double v_len2 = v_x * v_x + v_y * v_y;
  const double proj_ratio = u_dot_v / v_len2;
  const double sq_distance = u_len2 - (u_dot_v * u_dot_v) / v_len2;

  return CollisionResult(sq_distance, proj_ratio);
}

}  // namespace model
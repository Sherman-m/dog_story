#include "dog.h"

namespace model {

// Вызывает внутри себя bag_.reserve(bag_capacity_)
Dog::Dog(Id id, std::string name, const std::pair<Point, const Road*>& pos,
         std::uint32_t bag_capacity)
    : id_(id),
      name_(std::move(name)),
      curr_pos_(pos.first),
      curr_road_(*pos.second),
      bag_capacity_(bag_capacity) {
  bag_.reserve(bag_capacity_);
}

const Dog::Id& Dog::GetId() const noexcept { return id_; }

const std::string& Dog::GetName() const noexcept { return name_; }

Dimension Dog::GetWidth() const noexcept { return width_; }

Point Dog::GetCurrentPosition() const noexcept { return curr_pos_; }

Point Dog::GetPreviousPosition() const noexcept { return prev_pos_; }

Speed Dog::GetSpeed() const noexcept { return speed_; }

Direction Dog::GetDirection() const noexcept { return direction_; }

Road Dog::GetCurrentRoad() const noexcept { return curr_road_; }

std::uint32_t Dog::GetBagCapacity() const noexcept { return bag_capacity_; }

const Dog::Bag& Dog::GetBag() const noexcept { return bag_; }

// Показателем, что рюкзак заполнен, является условие
// bag_.size() == bag_.capacity().
bool Dog::PutInBag(const model::LostObject& lost_object) {
  if (bag_.size() == bag_capacity_) {
    return false;
  }
  bag_.push_back(lost_object);
  return true;
}

void Dog::HandOverLoot() {
  if (bag_.empty()) {
    return;
  }
  std::uint32_t points = 0;
  for (auto& lost_object : bag_) {
    points += lost_object.GetValue();
  }
  AddScore(points);
  bag_.clear();
}

Dog::Milliseconds Dog::GetTimeInGame() const noexcept { return time_in_game_; }

void Dog::AddTimeInGame(model::Dog::Milliseconds time_delta) {
  time_in_game_ += time_delta;
}

Dog::Milliseconds Dog::GetIdleTime() const noexcept { return idle_time_; }

void Dog::AddIdleTime(Milliseconds time_delta) { idle_time_ += time_delta; }

void Dog::ResetIdleTime() { idle_time_ = Milliseconds(0); }

bool Dog::IsMovingNow() const noexcept {
  return speed_.sx != 0 || speed_.sy != 0;
}

std::uint32_t Dog::GetScore() const noexcept { return score_; }

void Dog::AddScore(std::uint32_t points) { score_ += points; }

// Находит next_pos собаки и обрабатывает ситуации ее расположения:
//  - Если next_pos находится в пределах curr_road_, то собака просто
//  перемещается next_pos и не меняет дорогу.
//  - Если next_pos находится за пределами curr_road_, и удалось найти новую
//  дорогу с помощью Map::GetRoadFromTo, то:
//   -- Если собака не уперлась в границу новой дороги, то собака перемещается в
//    next_pos и меняет дорогу.
//   -- Если собака уперлась в границу новой дороги, то собака меняет дорогу и
//    встает возле ее границы.
//  - Если не удалось найти новую дорогу, то встаем у границы curr_road_.
void Dog::UpdatePosition(const Map* map, Milliseconds time_delta) {
  using namespace std::chrono;
  const double time_delta_sec = duration<double>(time_delta).count();
  auto next_pos = Point(curr_pos_.x + speed_.sx * time_delta_sec,
                        curr_pos_.y + speed_.sy * time_delta_sec);
  if (curr_road_.IsInsideRoad(next_pos)) {
    SetPosition(next_pos);
  } else if (auto new_point_and_road = map->GetRoadFromTo(curr_pos_, next_pos);
             new_point_and_road.second) {
    SetRoad(*new_point_and_road.second);
    if (new_point_and_road.first == next_pos) {
      SetPosition(new_point_and_road.first);
    } else {
      SetBoundaries(next_pos, std::make_pair(new_point_and_road.first,
                                             new_point_and_road.first));
    }
  } else {
    SetBoundaries(next_pos, std::make_pair(curr_road_.GetStartPosition(),
                                           curr_road_.GetEndPosition()));
  }
}

void Dog::SetSpeed(const Speed& speed) { speed_ = speed; }

void Dog::SetDirection(const Direction& direction) { direction_ = direction; }

void Dog::SetPosition(const Point& pos) {
  prev_pos_ = curr_pos_;
  curr_pos_ = pos;
}

void Dog::SetRoad(const model::Road& road) { curr_road_ = road; }

void Dog::SetBoundaries(const Point& unchanged_point,
                        const std::pair<Point, Point>& pos) {
  auto half_road_width = curr_road_.GetWidth() / 2;
  SetSpeed(Speed(0, 0));
  ResetIdleTime();
  switch (direction_) {
    case Direction::kNorth:
      SetPosition(Point(unchanged_point.x,
                        std::min(pos.first.y, pos.first.y) - half_road_width));
      break;
    case Direction::kSouth:
      SetPosition(
          Point(unchanged_point.x,
                std::max(pos.second.y, pos.second.y) + half_road_width));
      break;
    case Direction::kWest:
      SetPosition(Point(std::min(pos.second.x, pos.second.x) - half_road_width,
                        unchanged_point.y));
      break;
    case Direction::kEast:
      SetPosition(Point(std::max(pos.second.x, pos.second.x) + half_road_width,
                        unchanged_point.y));
      break;
  }
}

}  // namespace model
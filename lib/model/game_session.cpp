#include "game_session.h"

#include "retired_dog.h"

namespace model {

GameSession::GameSession(Id id, std::string name, Map::Id map_id)
    : id_(id), name_(std::move(name)), map_id_(std::move(map_id)) {}

const GameSession::Id& GameSession::GetId() const noexcept { return id_; }

const std::string& GameSession::GetName() const noexcept { return name_; }

const Map::Id& GameSession::GetMapId() const noexcept { return map_id_; }

GameSession::Dogs GameSession::GetDogs() {
  Dogs dogs;
  dogs.reserve(dog_id_to_dog_.size());
  for (auto& dog : dog_id_to_dog_) {
    dogs.push_back(&dog.second);
  }
  return dogs;
}

GameSession::ConstDogs GameSession::GetDogs() const {
  ConstDogs dogs;
  dogs.reserve(dog_id_to_dog_.size());
  for (auto& dog : dog_id_to_dog_) {
    dogs.push_back(&dog.second);
  }

  return dogs;
}

std::uint32_t GameSession::GetDogsCount() const noexcept {
  return dog_id_to_dog_.size();
}

Dog* GameSession::GetDogById(const Dog::Id& dog_id) {
  if (auto it = dog_id_to_dog_.find(dog_id); it != dog_id_to_dog_.end()) {
    return &(it->second);
  }
  return nullptr;
}

const Dog* GameSession::GetDogById(const Dog::Id& dog_id) const {
  if (auto it = dog_id_to_dog_.find(dog_id); it != dog_id_to_dog_.end()) {
    return &(it->second);
  }
  return nullptr;
}

Dog* GameSession::GetDogByName(const std::string& dog_name) {
  for (auto& dog : dog_id_to_dog_) {
    if (dog.second.GetName() == dog_name) {
      return &dog.second;
    }
  }
  return nullptr;
}

const Dog* GameSession::GetDogByName(const std::string& dog_name) const {
  for (auto& dog : dog_id_to_dog_) {
    if (dog.second.GetName() == dog_name) {
      return &dog.second;
    }
  }
  return nullptr;
}

bool GameSession::IsEmpty() const noexcept { return dog_id_to_dog_.empty(); }

Dog* GameSession::AddDog(std::string dog_name,
                         const std::pair<Point, const Road*>& dog_pos,
                         std::uint32_t bag_capacity) {
  Dog dog(Dog::Id(++next_dog_id_), std::move(dog_name), dog_pos, bag_capacity);
  try {
    auto [it, inserted] = dog_id_to_dog_.emplace(dog.GetId(), std::move(dog));
    return &it->second;
  } catch (...) {
    --next_dog_id_;
    throw std::runtime_error(
        "Failed to add dog with id == "s + std::to_string(next_dog_id_ + 1) +
        "in game session with id == "s + std::to_string(*id_));
  }
}

// Так как персонаж уже был сконструирован ранее, то у него уже имеется свой id.
// Для того чтобы id персонажей были уникальными, нужно при добавлении персонажа
// изменить next_dog_id_.
Dog* GameSession::LoadDog(Dog dog) {
  Dog::Id dog_id(dog.GetId());
  std::uint32_t temp_next_dog_id = next_dog_id_;
  try {
    auto [it, inserted] = dog_id_to_dog_.emplace(dog_id, std::move(dog));
    next_dog_id_ = std::max(next_dog_id_, *it->first);
    return &it->second;
  } catch (...) {
    next_dog_id_ = temp_next_dog_id;
    throw std::runtime_error(
        "Failed to load dog with id == "s + std::to_string(next_dog_id_ + 1) +
        "in game session with id == "s + std::to_string(*id_));
  }
}

void GameSession::MoveDog(const Dog::Id& dog_id, const Speed& dog_speed_on_map,
                          const std::string& movement) {
  if (auto dog = GetDogById(dog_id)) {
    if (movement.empty()) {
      dog->SetSpeed(model::Speed(0, 0));
      dog->ResetIdleTime();
      return;
    }
    switch (static_cast<model::Direction>(movement[0])) {
      case model::Direction::kNorth:
        dog->SetDirection(model::Direction::kNorth);
        dog->SetSpeed(model::Speed(0, -dog_speed_on_map.sy));
        return;
      case model::Direction::kSouth:
        dog->SetDirection(Direction::kSouth);
        dog->SetSpeed(Speed(0, dog_speed_on_map.sy));
        return;
      case Direction::kWest:
        dog->SetDirection(Direction::kWest);
        dog->SetSpeed(Speed(-dog_speed_on_map.sx, 0));
        return;
      case Direction::kEast:
        dog->SetDirection(Direction::kEast);
        dog->SetSpeed(Speed(dog_speed_on_map.sx, 0));
        return;
    }
  } else {
    throw std::invalid_argument("Dog with id"s + std::to_string(*dog_id) +
                                "does not exist"s);
  }
}

const GameSession::Loot& GameSession::GetLoot() const noexcept { return loot_; }

std::uint32_t GameSession::GetLootCount() const noexcept {
  return loot_.size();
}

void GameSession::AddLoot(const Map* map, std::uint32_t loot_count) {
  using namespace std::literals;
  while (loot_count--) {
    ++next_lost_object_id_;
    try {
      std::uint32_t num_of_loot_type = map->GenerateRandomLootType();
      std::uint64_t value =
          json_loader::LootTypesStorage::GetTypesOfLoots(map_id_)
              ->at(num_of_loot_type)
              .as_object()
              .at("value"s)
              .as_int64();
      LostObject lost_object(LostObject::Id(next_lost_object_id_),
                             num_of_loot_type,
                             map->GenerateRandomPosition().first,
                             static_cast<std::uint32_t>(value));
      loot_.push_back(lost_object);
    } catch (...) {
      --next_lost_object_id_;
      throw std::runtime_error("Failed to add lost object with id == "s +
                               std::to_string(next_lost_object_id_ + 1) +
                               "in game session with id == "s +
                               std::to_string(*id_));
    }
  }
}

// Так как потерянная вещь уже была сконструирована ранее, то у нее уже имеется
// свой id. Для того чтобы id потерянных вещей были уникальными, нужно при
// добавлении потерянной вещи изменить next_lost_object_id_.
LostObject* GameSession::LoadLostObject(const LostObject& lost_object) {
  std::uint32_t temp_next_lost_object_id = next_lost_object_id_;
  try {
    auto& inserted_lost_object = loot_.emplace_back(lost_object);
    next_lost_object_id_ = std::max(next_lost_object_id_, *lost_object.GetId());
    return &inserted_lost_object;
  } catch (...) {
    next_lost_object_id_ = temp_next_lost_object_id;
    throw std::runtime_error("Failed to load lost object with id == "s +
                             std::to_string(*lost_object.GetId()) +
                             " in game session with id == "s +
                             std::to_string(*id_));
  }
}
// Во время обновления игрового состояния собак проверяет, двигается ли собака в
// данный момент и, если собака не двигается, сколько времени прошло с
// последнего вызова Dog::SetIdleTimeNow(). Если собака не двигается больше, чем
// map->GetDogRetirementTime(), то собака считается ушедшей на покой: ее данные
// добавляются в вектор ушедших на покой собак, а она сама удаляется из игровой
// сессии.
GameSession::RetiredDogs GameSession::UpdateSession(const Map* map,
                                                    std::uint32_t loot_count,
                                                    Milliseconds time_delta) {
  using namespace std::chrono;
  AddLoot(map, loot_count);
  RetiredDogs retired_dogs;
  for (auto& [dog_id, dog] : dog_id_to_dog_) {
    dog.AddTimeInGame(time_delta);
    if (!dog.IsMovingNow()) {
      dog.AddIdleTime(time_delta);
      if (dog.GetIdleTime() >= map->GetDogRetirementTime()) {
        retired_dogs.emplace_back(
            dog_id, dog.GetName(), dog.GetScore(),
            dog.GetTimeInGame(),
            id_);
      }
    } else {
      dog.UpdatePosition(map, time_delta);
    }
  }
  DeleteRetiredDogs(retired_dogs);
  auto events = FindCollisionEvents(map->GetOffices());
  HandleCollisions(events);
  return retired_dogs;
}

void GameSession::DeleteRetiredDogs(const RetiredDogs& retired_dogs) {
  for (auto& dog : retired_dogs) {
    dog_id_to_dog_.erase(dog.GetId());
  }
}

// При нахождении события столкновения Dog и LostObject, CollisionEvent
// создается со следующими параметрами:
//  - type - CollisionEventType::kCollect;
//  - lost_object_ptr - <указатель на объект LostObject, созданный в куче на
//                       основе объекта, хранящегося в loot_>;
//  - lost_object_pos - <итератор на объект LostObject, хранящийся в loot_>.
//
// При нахождении события столкновения Dog и Office, CollisionEvent
// создается со следующими параметрами:
//  - type - CollisionEventType::kPass;
//  - lost_object_ptr - nullptr;
//  - lost_object_pos - loot_.end().
GameSession::CollisionEvents GameSession::FindCollisionEvents(
    const std::vector<Office>& offices) {
  std::vector<CollisionEvent> detected_collisions;
  for (const auto& [dog_id, dog] : dog_id_to_dog_) {
    for (auto lost_object_iter = loot_.begin(); lost_object_iter != loot_.end();
         lost_object_iter = std::next(lost_object_iter)) {
      auto lost_object_ptr = std::make_shared<LostObject>(*lost_object_iter);
      if (auto collection_result = TryCollectPoint(
              dog.GetPreviousPosition(), dog.GetCurrentPosition(),
              lost_object_ptr->GetPosition());
          collection_result.IsCollected(dog.GetWidth() / 2)) {
        detected_collisions.emplace_back(CollisionEventType::kCollect, dog_id,
                                         lost_object_ptr, lost_object_iter,
                                         collection_result.sq_distance,
                                         collection_result.proj_ratio);
      }
    }
    for (const auto& office : offices) {
      if (auto collection_result =
              TryCollectPoint(dog.GetPreviousPosition(),
                              dog.GetCurrentPosition(), office.GetPosition());
          collection_result.IsCollected(dog.GetWidth() / 2 +
                                        office.GetWidth() / 2)) {
        detected_collisions.emplace_back(
            CollisionEventType::kPass, dog_id, nullptr, loot_.end(),
            collection_result.sq_distance, collection_result.proj_ratio);
      }
    }
  }
  std::sort(detected_collisions.begin(), detected_collisions.end());
  return detected_collisions;
}

// При обработке события столкновения с типом CollisionEventType::kCollect
// удаляет объект LostObject из loot_, если она добавляется в рюкзак собаки.
//
// При обработке события столкновения с типом CollisionEventType::kPass
// вызывает функцию-член HandOverLoot() объекта dog.
void GameSession::HandleCollisions(const GameSession::CollisionEvents& events) {
  for (auto& event : events) {
    auto dog = GetDogById(event.dog_id);
    if (event.type == CollisionEventType::kCollect &&
        !event.lost_object_ptr->IsCollected()) {
      if (dog->PutInBag(*event.lost_object_ptr)) {
        event.lost_object_ptr->Collect();
        loot_.erase(event.lost_object_pos);
      }
    } else if (event.type == CollisionEventType::kPass) {
      dog->HandOverLoot();
    }
  }
}

}  // namespace model
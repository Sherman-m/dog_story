#pragma once

#include <chrono>
#include <list>
#include <string>
#include <unordered_map>
#include <vector>

#include "../json_loader/loot_types_storage.h"
#include "../util/tagged.h"
#include "collision_detector.h"
#include "dog.h"
#include "map.h"

namespace model {

// Описывает объект игровой сессии.
// Управляет жизненным циклом собак, участвующих в игре, и потерянных вещей,
// находящихся в игровой сессии.
//
// В некоторых функциях-членах используются сырые указатели на Dog, так как мы
// не продлеваем жизнь объекта, а только предоставляем доступ для
// взаимодействия с объектом.
class GameSession {
 public:
  using Id = util::Tagged<std::uint32_t, GameSession>;
  using Dogs = std::vector<Dog*>;
  // Для хранения потерянных вещей используем std::list, чтобы быстро удалять и
  // добавлять потерянные вещи.
  using Loot = std::list<LostObject>;
  using CollisionEvents = std::vector<CollisionEvent>;
  using Milliseconds = std::chrono::milliseconds;

  explicit GameSession(Id id, std::string name, Map::Id map_id);

  const Id& GetId() const noexcept;

  const std::string& GetName() const noexcept;

  const Map::Id& GetMapId() const noexcept;

  Dogs GetDogs();

  Dog* GetDogById(const Dog::Id& dog_id) noexcept;

  const Dog* GetDogById(const Dog::Id& dog_id) const noexcept;

  Dog* AddDog(std::string dog_name,
              const std::pair<Point, const Road*>& dog_pos,
              std::uint32_t bag_capacity);

  void MoveDog(const Dog::Id& dog_id, const Speed& dog_speed_on_map,
               const std::string& movement);

  const Loot& GetLoot() const noexcept;

  //  Генерирует loot_count потерянных объектов.
  //  В качестве входных данным передаем Map*, чтобы получить доступ к
  //  генератору типа потерянного объекта.
  void AddLoot(const Map* map, std::uint32_t loot_count);

  // Обновляет игровое состояние сессии, включая генерацию лута, обновление
  // позиции собак, и обработку событий столкновения.
  void UpdateSession(const Map* map, std::uint32_t loot_count,
                     Milliseconds time_delta);

 private:
  using DogIdHasher = util::TaggedHasher<Dog::Id>;
  using DogIdToDog = std::unordered_map<Dog::Id, Dog, DogIdHasher>;

  // Находит события столкновения собак с потерянными предметами и офисами и
  // возвращает эти события, отсортированные в хронологическом порядке.
  CollisionEvents FindCollisionEvents(const std::vector<Office>& offices);

  // Обрабатывает события столкновения.
  void HandleCollisions(const CollisionEvents& events);

  Id id_;
  std::string name_;
  Map::Id map_id_;
  DogIdToDog dog_id_to_dog_;
  std::uint32_t next_dog_id_ = 0;
  Loot loot_;
  std::uint32_t next_lost_object_id_ = 0;
};

}  // namespace model
#pragma once

#include <chrono>
#include <pqxx/pqxx>
#include <vector>

#include "../../lib/model/game_session.h"
#include "../../lib/model/retired_dog.h"
#include "../../lib/util/tagged_uuid.h"

namespace db {

using namespace std::literals;
using pqxx::operator"" _zv;

using RetiredPlayerId = util::TaggedUUID<model::RetiredDog>;

class RetiredPlayersRepository {
 public:
  using Milliseconds = std::chrono::milliseconds;

  explicit RetiredPlayersRepository(pqxx::work& work);

  void SaveRetiredPlayers(const model::GameSession::RetiredDogs& retired_dogs);

  model::GameSession::RetiredDogs GetRetiredPlayers(std::uint32_t offset,
                                                    std::uint32_t max_items);

 private:
  pqxx::work& work_;
};
}  // namespace db
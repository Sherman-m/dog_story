#include "retired_players_repository.h"

namespace db {

RetiredPlayersRepository::RetiredPlayersRepository(pqxx::work& work)
    : work_(work) {}

void RetiredPlayersRepository::SaveRetiredPlayers(
    const model::GameSession::RetiredDogs& retired_dogs) {
  for (auto& dog : retired_dogs) {
    work_.exec_params(R"(
INSERT INTO retired_players (id, name, score, play_time_ms)
VALUES ($1, $2, $3, $4)
ON CONFLICT (id) DO UPDATE SET name = $2, score = $3, play_time_ms = $4;)"_zv,
                      RetiredPlayerId::New().ToString(), dog.GetName(),
                      dog.GetScore(), dog.GetPlayTime().count());
  }
}

model::GameSession::RetiredDogs RetiredPlayersRepository::GetRetiredPlayers(
    std::uint32_t offset, std::uint32_t max_items) {
  auto query_result = work_.exec_params(R"(
SELECT name, score, play_time_ms
  FROM retired_players
 ORDER BY score DESC, play_time_ms ASC, name ASC
 LIMIT $1
OFFSET $2)"_zv,
                                        max_items, offset);
  model::GameSession::RetiredDogs retired_dogs;
  for (auto row : query_result) {
    auto [name, score, play_time_ms] =
        row.as<std::string, std::uint32_t, std::uint32_t>();
    retired_dogs.emplace_back(name, score, Milliseconds(play_time_ms));
  }
  return retired_dogs;
}

}  // namespace db
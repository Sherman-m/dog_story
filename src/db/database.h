#pragma once

#include <cinttypes>
#include <string>

#include "connection_pool.h"
#include "unit_of_work.h"

namespace db {

template <typename ConnectionFactory>
struct DatabaseConfig {
  explicit DatabaseConfig(std::uint32_t count, ConnectionFactory&& factory)
      : connection_count(count), connection_factory(std::move(factory)) {}

  std::uint32_t connection_count = 1;
  ConnectionFactory connection_factory;
};

class Database {
 public:
  template <typename ConnectionFactory>
  explicit Database(const DatabaseConfig<ConnectionFactory>& config)
      : connection_pool_(config.connection_count, config.connection_factory) {
    using namespace std::literals;
    using pqxx::operator""_zv;

    auto connection = connection_pool_.GetConnection();
    pqxx::work work(*connection);
    work.exec(R"(
CREATE TABLE IF NOT EXISTS retired_players (
  id UUID CONSTRAINT book_id_constraint PRIMARY KEY,
  name varchar(100) NOT NULL,
  score integer NOT NULL,
  play_time_ms integer NOT NULL
);
)"_zv);
    work.exec(R"(
CREATE INDEX IF NOT EXISTS name_score_play_time_ms_idx
ON retired_players (name, score, play_time_ms);
)"_zv);
    work.commit();
  }

  UnitOfWork CreateUnitOfWork();

 private:
  ConnectionPool connection_pool_;
};

}  // namespace db
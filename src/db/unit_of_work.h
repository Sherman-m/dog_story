#pragma once

#include <pqxx/pqxx>

#include "connection_pool.h"
#include "retired_players_repository.h"

namespace db {

// Реализация паттерна UnitOfWork.
// Уникально владеет подключением к базе данных.
class UnitOfWork {
 public:
  explicit UnitOfWork(ConnectionPool::ConnectionWrapper&& connection);

  RetiredPlayersRepository& GetRetiredPlayersRepository() &;

  void Commit();

 private:
  ConnectionPool::ConnectionWrapper connection_;
  pqxx::work work_{*connection_};
  RetiredPlayersRepository retired_players_{work_};
};

}  // namespace db
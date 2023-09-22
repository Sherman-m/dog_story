#include "unit_of_work.h"

namespace db {

UnitOfWork::UnitOfWork(ConnectionPool::ConnectionWrapper&& connection)
    : connection_(std::move(connection)) {}

RetiredPlayersRepository& UnitOfWork::GetRetiredPlayersRepository() & {
  return retired_players_;
}

void UnitOfWork::Commit() { work_.commit(); }
}  // namespace db
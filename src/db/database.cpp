#include "database.h"

namespace db {

UnitOfWork Database::CreateUnitOfWork() {
  return UnitOfWork(connection_pool_.GetConnection());
}

}  // namespace db
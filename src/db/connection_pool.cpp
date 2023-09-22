#include "connection_pool.h"

namespace db {

ConnectionPool::ConnectionWrapper ConnectionPool::GetConnection() {
  using namespace std::literals;
  if (id_of_connection_owners_threads.find(std::this_thread::get_id()) !=
      id_of_connection_owners_threads.end()) {
    throw std::runtime_error("This thread already has connection"s);
  }
  std::unique_lock lock(mutex_);
  cond_var_.wait(lock, [this] { return used_connections_ < pool_.size(); });

  id_of_connection_owners_threads.insert(std::this_thread::get_id());
  return ConnectionWrapper(std::move(pool_[used_connections_++]), this);
}

void ConnectionPool::ReturnConnection(
    db::ConnectionPool::ConnectionPtr&& connection_ptr) {
  {
    std::lock_guard lock(mutex_);
    assert(used_connections_ != 0);
    pool_[--used_connections_] = std::move(connection_ptr);
    id_of_connection_owners_threads.erase(std::this_thread::get_id());
  }
  cond_var_.notify_one();
}

}  // namespace db
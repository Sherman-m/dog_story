#pragma once

#include <cinttypes>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <pqxx/pqxx>
#include <string>
#include <thread>
#include <unordered_set>

namespace db {

// Реализация пула подключению к базе данных. Такой подход используется для
// экономии времени подключения к базе данных.
class ConnectionPool {
  using ConnectionPtr = std::shared_ptr<pqxx::connection>;
  using PoolType = ConnectionPool;
  using IdOfConnectionOwnersThreads = std::unordered_set<std::jthread::id>;

 public:
  class ConnectionWrapper {
   public:
    explicit ConnectionWrapper(ConnectionPtr&& connection_ptr, PoolType* pool)
        : connection_ptr_(std::move(connection_ptr)), pool_(pool) {}

    ConnectionWrapper(const ConnectionWrapper&) = delete;
    ConnectionWrapper& operator=(const ConnectionWrapper&) = delete;

    ConnectionWrapper(ConnectionWrapper&&) = default;
    ConnectionWrapper& operator=(ConnectionWrapper&&) = default;

    ~ConnectionWrapper() {
      if (connection_ptr_) {
        pool_->ReturnConnection(std::move(connection_ptr_));
      }
    }

    pqxx::connection& operator*() const& noexcept { return *connection_ptr_; }

    pqxx::connection& operator*() const&& = delete;

    pqxx::connection* operator->() const& noexcept {
      return connection_ptr_.get();
    }

   private:
    ConnectionPtr connection_ptr_;
    PoolType* pool_;
  };

  template <typename ConnectionFactory>
  explicit ConnectionPool(std::uint32_t capacity,
                          ConnectionFactory&& connection_factory) {
    pool_.reserve(capacity);
    for (std::uint32_t i = 0; i < capacity; ++i) {
      pool_.emplace_back(connection_factory());
    }
  }

  ConnectionWrapper GetConnection();

 private:
  void ReturnConnection(ConnectionPtr&& connection_ptr);

  std::mutex mutex_;
  std::condition_variable cond_var_;
  std::vector<ConnectionPtr> pool_;
  IdOfConnectionOwnersThreads id_of_connection_owners_threads;  // Guarded by
                                                                // mutex_
  std::uint32_t used_connections_ = 0;
};

}  // namespace db
#include "strand_storage.h"

namespace app {

StrandStorage::StrandStorage(net::io_context& ioc) : ioc_(ioc) {}

const StrandStorage::Strand* StrandStorage::GetStrand(
    const model::GameSession::Id& game_session_id) const noexcept {
  if (auto it = storage_.find(game_session_id); it != storage_.end()) {
    return &it->second;
  }
  return nullptr;
}

// При добавлении игровой сессии генерирует нового последовательного
// исполнителя.
const StrandStorage::Strand* StrandStorage::AddStrand(
    const model::GameSession::Id& game_session_id) {
  using namespace std::literals;
  try {
    auto [it, inserted] =
        storage_.emplace(game_session_id, net::make_strand(ioc_));
    return &it->second;
  } catch (...) {
    throw;
  }
}

}  // namespace app
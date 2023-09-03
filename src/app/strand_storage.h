#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>

#include "../../lib/model/game_session.h"
#include "../../lib/util/tagged.h"

namespace app {

namespace net = boost::asio;

// Хранит последовательных исполнителей для игровых сессий.
class StrandStorage {
 public:
  using Strand = net::strand<net::io_context::executor_type>;

  explicit StrandStorage(net::io_context& ioc);
  StrandStorage(const StrandStorage&) = delete;
  StrandStorage& operator=(const StrandStorage&) = delete;

  const Strand* GetStrand(
      const model::GameSession::Id& game_session_id) const noexcept;
  const Strand* AddStrand(const model::GameSession::Id& game_session_id);

 private:
  using GameSessionToStrand =
      std::unordered_map<model::GameSession::Id, Strand,
                         util::TaggedHasher<model::GameSession::Id>>;

  GameSessionToStrand storage_;
  net::io_context& ioc_;
};

}  // namespace app
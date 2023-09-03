#pragma once

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/date_time.hpp>
#include <chrono>
#include <memory>
#include <utility>

#include "../app/application.h"

namespace app {

namespace net = boost::asio;
namespace sys = boost::system;

// Класс Ticker обновляет игровое состояние через заданный tick_period.
class Ticker : public std::enable_shared_from_this<Ticker> {
 public:
  using Strand = net::strand<net::io_context::executor_type>;
  using Milliseconds = std::chrono::milliseconds;
  using Clock = std::chrono::steady_clock;
  using Handler =
      std::function<bool(const model::GameSession::Id&, Milliseconds)>;

  explicit Ticker(const model::GameSession::Id& game_session_id, Strand strand,
                  Milliseconds period, Handler handler);
  Ticker(const Ticker&) = delete;
  Ticker& operator=(const Ticker&) = delete;

  void Start();

 private:
  void ScheduleTick();
  void OnTick(sys::error_code ec);

  model::GameSession::Id game_session_id_;
  Strand game_session_strand_;
  net::steady_timer timer_{game_session_strand_};
  Milliseconds period_;
  Handler handler_;
  Clock::time_point last_tick_;
};

}  //  namespace app
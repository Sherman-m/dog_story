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

// Класс Ticker обновляет игровое состояние через заданный tick_period и
// сохраняет это состояние в файл.
class Ticker : public std::enable_shared_from_this<Ticker> {
 public:
  using Timer = net::steady_timer;
  using Milliseconds = std::chrono::milliseconds;
  using UpdateHandler = std::function<bool(Milliseconds time_delta)>;
  using SaveHandler = std::function<bool()>;
  using Clock = std::chrono::steady_clock;

  explicit Ticker(net::io_context& ioc, Milliseconds update_period,
                  bool is_save_state_period_set, Milliseconds save_state_period,
                  UpdateHandler update_handler, SaveHandler save_handler);
  Ticker(const Ticker&) = delete;
  Ticker& operator=(const Ticker&) = delete;

  void Start();

 private:
  void ScheduleTick();
  void OnTick(sys::error_code ec);

  Timer timer_;
  Milliseconds update_period_;
  bool is_save_state_period_set_;
  Milliseconds save_state_period_;
  UpdateHandler update_handler_;
  SaveHandler save_handler_;
  Clock::time_point last_update_tick_;
  Clock::time_point last_save_tick_;
};

}  //  namespace app
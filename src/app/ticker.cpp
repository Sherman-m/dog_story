#include "ticker.h"

namespace app {

Ticker::Ticker(net::io_context& ioc, Milliseconds update_period,
               bool is_save_state_period_set, Milliseconds save_state_period,
               UpdateHandler update_handler, SaveHandler save_handler)
    : timer_(ioc),
      update_period_(update_period),
      is_save_state_period_set_(is_save_state_period_set),
      save_state_period_(save_state_period),
      update_handler_(std::move(update_handler)),
      save_handler_(std::move(save_handler)) {}

void Ticker::Start() {
  last_update_tick_ = Clock::now();
  last_save_tick_ = Clock::now();
  ScheduleTick();
}

void Ticker::ScheduleTick() {
  timer_.expires_after(update_period_);
  timer_.async_wait([self = this->shared_from_this()](sys::error_code ec) {
    self->OnTick(ec);
  });
}

void Ticker::OnTick(sys::error_code ec) {
  using namespace std::chrono;
  if (!ec) {
    auto current_tick = Clock::now();
    auto time_delta =
        duration_cast<Milliseconds>(current_tick - last_update_tick_);
    last_update_tick_ = current_tick;
    update_handler_(time_delta);
    if (is_save_state_period_set_ &&
        duration_cast<Milliseconds>(current_tick - last_save_tick_) >=
            save_state_period_) {
      save_handler_();
    }
    ScheduleTick();
  }
}

}  //  namespace app
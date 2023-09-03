#include "ticker.h"

namespace app {

Ticker::Ticker(const model::GameSession::Id& game_session_id, Strand strand,
               Milliseconds period, Handler handler)
    : game_session_id_(game_session_id),
      game_session_strand_(std::move(strand)),
      period_(period),
      handler_(std::move(handler)) {}

void Ticker::Start() {
  net::dispatch(game_session_strand_, [self = this->shared_from_this()] {
    self->last_tick_ = Clock::now();
    self->ScheduleTick();
  });
}

void Ticker::ScheduleTick() {
  timer_.expires_after(period_);
  timer_.async_wait(net::bind_executor(
      game_session_strand_, [self = this->shared_from_this()](
                                sys::error_code ec) { self->OnTick(ec); }));
}

void Ticker::OnTick(sys::error_code ec) {
  using namespace std::chrono;
  if (!ec) {
    auto current_tick = Clock::now();
    auto time_delta = duration_cast<milliseconds>(current_tick - last_tick_);
    last_tick_ = current_tick;
    try {
      handler_(game_session_id_, time_delta);
    } catch (...) {
    }
    ScheduleTick();
  }
}

}  //  namespace app
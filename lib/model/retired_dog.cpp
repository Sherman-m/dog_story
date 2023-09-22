#include "retired_dog.h"

namespace model {

RetiredDog::RetiredDog(Dog::Id id, std::string dog_name,
                       std::uint32_t dog_score,
                       model::RetiredDog::Milliseconds play_time,
                       GameSession::Id game_session_id)
    : id_(id),
      name_(std::move(dog_name)),
      score_(dog_score),
      play_time_(play_time),
      game_session_id_(game_session_id) {}

// Так как данные об id собаки и id игровой сессии не хранятся в БД, то
// используется облегченный формат представления собаки, ушедшей на покой.
RetiredDog::RetiredDog(std::string dog_name, std::uint32_t dog_score,
                       Milliseconds play_time)
    : id_(0),
      name_(std::move(dog_name)),
      score_(dog_score),
      play_time_(play_time),
      game_session_id_(0) {}

const Dog::Id& RetiredDog::GetId() const noexcept { return id_; }

const std::string& RetiredDog::GetName() const noexcept { return name_; }

std::uint32_t RetiredDog::GetScore() const noexcept { return score_; }

RetiredDog::Milliseconds RetiredDog::GetPlayTime() const noexcept {
  return play_time_;
}

const GameSession::Id& RetiredDog::GetGameSessionId() const noexcept {
  return game_session_id_;
}
}  // namespace model
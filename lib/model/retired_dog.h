#pragma once

#include <chrono>
#include <cinttypes>
#include <string>

#include "game_session.h"

namespace model {

// Описывает данные собаки, которая отправилась на покой.
// Данный класс будет использоваться для добавления данных об игроке в БД.
class RetiredDog {
 public:
  using Milliseconds = std::chrono::milliseconds;

  explicit RetiredDog(Dog::Id id, std::string dog_name, std::uint32_t dog_score,
                      Milliseconds play_time, GameSession::Id game_session_id);

  explicit RetiredDog(std::string dog_name, std::uint32_t dog_score,
                      Milliseconds play_time);

  const Dog::Id& GetId() const noexcept;

  const std::string& GetName() const noexcept;

  std::uint32_t GetScore() const noexcept;

  Milliseconds GetPlayTime() const noexcept;

  const GameSession::Id& GetGameSessionId() const noexcept;

 private:
  Dog::Id id_;
  std::string name_;
  std::uint32_t score_;
  Milliseconds play_time_;
  GameSession::Id game_session_id_;
};

}  // namespace model
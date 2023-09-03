#pragma once

#include <cstdint>

#include "../util/tagged.h"
#include "geometry.h"

namespace model {

// Описывает потерянный предмет
class LostObject {
 public:
  using Id = util::Tagged<std::uint32_t, LostObject>;

  explicit LostObject(Id id, std::uint32_t type, const Point& pos,
                      std::uint32_t value);

  const Id& GetId() const noexcept;

  std::uint32_t GetType() const noexcept;

  Point GetPosition() const noexcept;

  std::uint32_t GetValue() const noexcept;

  bool IsCollected() const noexcept;

  void Collect();

 private:
  Id id_;
  std::uint32_t type_;
  std::uint32_t value_ = 1;
  Point pos_;
  bool is_collected_ = false;
};

}  // namespace model
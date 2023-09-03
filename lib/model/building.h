#pragma once

#include "geometry.h"

namespace model {

// Описывает объект здание в игре
class Building {
 public:
  explicit Building(const Rectangle& bounds) noexcept;

  Rectangle GetBounds() const noexcept;

 private:
  Rectangle bounds_;
};

}  // namespace model
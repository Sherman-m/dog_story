#pragma once

#include <stdexcept>
#include <string>

#include "../util/tagged.h"
#include "geometry.h"

namespace model {

// Описывает объект офис в игре
class Office {
 public:
  using Id = util::Tagged<std::string, Office>;

  explicit Office(Id id, const Point& pos, const Offset& offset) noexcept;

  const Id& GetId() const noexcept;
  Point GetPosition() const noexcept;
  Offset GetOffset() const noexcept;
  Dimension GetWidth() const noexcept;

 private:
  Id id_;
  Point pos_;
  Offset offset_;
  Dimension width_ = 0.5;
};

}  // namespace model
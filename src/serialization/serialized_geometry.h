#pragma once

#include <cinttypes>

#include "../../lib/model/geometry.h"
#include "../../lib/model/road.h"

namespace model {

// Представляет функции для сериализации тривиальных объектов игры, таких как
// точки, прямоугольники и т. д.

template <typename Archive>
void serialize(Archive& ar, Point& point,
               [[maybe_unused]] const std::uint32_t version) {
  ar& point.x;
  ar& point.y;
}

template <typename Archive>
void serialize(Archive& ar, Size& size,
               [[maybe_unused]] const std::uint32_t version) {
  ar& size.height;
  ar& size.width;
}

template <typename Archive>
void serialize(Archive& ar, Rectangle& rectangle,
               [[maybe_unused]] const std::uint32_t version) {
  ar& rectangle.pos;
  ar& rectangle.size;
}

template <typename Archive>
void serialize(Archive& ar, Offset& offset,
               [[maybe_unused]] const std::uint32_t version) {
  ar& offset.dx;
  ar& offset.dy;
}

template <typename Archive>
void serialize(Archive& ar, Speed& speed,
               [[maybe_unused]] const std::uint32_t version) {
  ar& speed.sx;
  ar& speed.sy;
}

}  // namespace model
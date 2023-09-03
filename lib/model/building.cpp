#include "building.h"

namespace model {

Building::Building(const Rectangle& bounds) noexcept : bounds_(bounds) {}

Rectangle Building::GetBounds() const noexcept { return bounds_; }

}  // namespace model
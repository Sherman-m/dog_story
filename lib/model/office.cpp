#include "office.h"

namespace model {

Office::Office(Office::Id id, const Point& pos, const Offset& offset) noexcept
    : id_(std::move(id)), pos_(pos), offset_(offset) {}

const Office::Id& Office::GetId() const noexcept { return id_; }

Point Office::GetPosition() const noexcept { return pos_; }

Offset Office::GetOffset() const noexcept { return offset_; }

Dimension Office::GetWidth() const noexcept { return width_; }

}  // namespace model
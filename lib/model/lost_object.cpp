#include "lost_object.h"

namespace model {

LostObject::LostObject(Id id, std::uint32_t type, const Point& pos,
                       std::uint32_t value)
    : id_(id), type_(type), pos_(pos), value_(value) {}

const LostObject::Id& LostObject::GetId() const noexcept { return id_; }

std::uint32_t LostObject::GetType() const noexcept { return type_; }

Point LostObject::GetPosition() const noexcept { return pos_; }

std::uint32_t LostObject::GetValue() const noexcept { return value_; }

bool LostObject::IsCollected() const noexcept { return is_collected_; }

void LostObject::Collect() { is_collected_ = true; }

}  // namespace model
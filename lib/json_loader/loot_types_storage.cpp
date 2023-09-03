#include "loot_types_storage.h"

namespace json_loader {

LootTypesStorage::LootTypes LootTypesStorage::loot_types_;

const json::array* LootTypesStorage::GetTypesOfLoots(
    const model::Map::Id& map_id) noexcept {
  if (auto loot_types = loot_types_.find(map_id);
      loot_types != loot_types_.end()) {
    return &loot_types->second;
  }
  return nullptr;
}

void LootTypesStorage::AddTypesOfLoots(const model::Map::Id& map_id,
                                       const boost::json::array& loot_types) {
  using namespace std::literals;
  auto [it, inserted] = loot_types_.emplace(map_id, loot_types);
  if (!inserted) {
    throw std::invalid_argument("Invalid types of loot"s);
  }
}

}  // namespace json_loader
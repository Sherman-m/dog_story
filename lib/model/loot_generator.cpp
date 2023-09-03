#include "loot_generator.h"

namespace model {

std::uint32_t LootGenerator::Generate(Milliseconds time_delta,
                                      std::uint32_t loot_count,
                                      std::uint32_t dog_count) {
  time_without_loot_ += time_delta;
  const std::uint32_t loot_shortage =
      loot_count > dog_count ? 0u : dog_count - loot_count;
  const double ratio =
      std::chrono::duration<double>{time_without_loot_} / base_interval_;
  const double probability = std::clamp(
      (1.0 - std::pow(1.0 - probability_, ratio)) * random_generator_(), 0.0,
      1.0);
  const auto generated_loot =
      static_cast<std::uint32_t>(std::round(loot_shortage * probability));
  if (generated_loot > 0) {
    time_without_loot_ = {};
  }
  return generated_loot;
}

}  // namespace model

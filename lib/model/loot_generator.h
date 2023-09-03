#pragma once

#include <chrono>
#include <cmath>
#include <functional>

namespace model {

// Описывает генератор потерянных вещей
class LootGenerator {
 public:
  using RandomGenerator = std::function<double()>;
  using Milliseconds = std::chrono::milliseconds;

  // base_interval - базовый отрезок времени > 0;
  // probability - вероятность появления потерянной вещи в течение базового
  //               интервала времени;
  // random_generator - генератор псевдослучайных чисел в диапазоне от
  // [0 до 1]
  LootGenerator(Milliseconds base_interval, double probability,
                RandomGenerator random_generator = DefaultGenerator)
      : base_interval_{base_interval},
        probability_{probability},
        random_generator_{std::move(random_generator)} {}

  // Возвращает количество потерянных вещей, которые должны появиться на карте
  // спустя заданный промежуток времени. Количество потерянных вещей,
  // появляющихся на карте не превышает количество собак.
  //
  // time_delta - отрезок времени, прошедший с момента предыдущего вызова
  // Generate;
  // loot_count - количество потерянных вещей на карте до вызова Generate;
  // dog_count - количество собак на карте;
  std::uint32_t Generate(Milliseconds time_delta, std::uint32_t loot_count,
                         std::uint32_t dog_count);

 private:
  static double DefaultGenerator() noexcept { return 1.0; };

  Milliseconds base_interval_;
  double probability_;
  Milliseconds time_without_loot_{};
  RandomGenerator random_generator_;
};

}  // namespace model

#pragma once

#include <iomanip>
#include <random>
#include <sstream>

#include "../../lib/util/tagged.h"

namespace app {

namespace detail {

struct TokenTag {};

}  // namespace detail

using Token = util::Tagged<std::string, detail::TokenTag>;
using TokenHasher = util::TaggedHasher<Token>;

// Отвечает за генерацию токенов авторизации.
class TokenGenerator {
 public:
  TokenGenerator() = default;
  TokenGenerator(TokenGenerator&) = delete;
  TokenGenerator& operator=(TokenGenerator&) = delete;
  TokenGenerator(TokenGenerator&&) = delete;
  TokenGenerator& operator=(TokenGenerator&&) = delete;

  // Генерирует уникальное 128-битное число.
  Token Generate();

 private:
  std::random_device random_device_;
  std::mt19937_64 generator1_{[this] {
    std::uniform_int_distribution<std::mt19937_64::result_type> dist;
    return dist(random_device_);
  }()};
  std::mt19937_64 generator2_{[this] {
    std::uniform_int_distribution<std::mt19937_64::result_type> dist;
    return dist(random_device_);
  }()};
};

}  // namespace app
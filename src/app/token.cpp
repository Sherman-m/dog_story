#include "token.h"

namespace app {

Token TokenGenerator::Generate() {
  std::ostringstream stream;
  std::uniform_int_distribution<std::mt19937_64::result_type> dist;
  stream << std::hex << std::setfill('0') << std::setw(16) << generator1_();
  stream << std::hex << std::setfill('0') << std::setw(16) << generator2_();
  return Token(stream.str());
}

}  // namespace app
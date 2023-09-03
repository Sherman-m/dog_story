#include "response_generators.h"

namespace http_handler {

std::string_view ContentType::ConvertExtensionToMimeType(
    std::string_view extension) noexcept {
  return mime_types_[extension];
}

}  // namespace http_handler

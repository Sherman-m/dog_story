#include "file_handler.h"

namespace util {

std::string LoadContentFromFile(const fs::path& path) {
  std::ifstream input_file(path, std::ios_base::in);
  if (!input_file.is_open()) {
    throw std::invalid_argument("Could not open the file - '"s + path.string());
  }
  return std::string{std::istreambuf_iterator<char>(input_file),
                     std::istreambuf_iterator<char>()};
}

bool IsSubPath(fs::path base_path, fs::path target_path) {
  base_path = fs::weakly_canonical(base_path);
  target_path = fs::weakly_canonical(target_path);
  for (auto base_path_it = base_path.begin(),
            target_path_it = target_path.begin();
       base_path_it != base_path.end(); ++base_path_it, ++target_path_it) {
    if (target_path_it == target_path.end() ||
        *target_path_it != *base_path_it) {
      return false;
    }
  }
  return true;
}

std::string DecodeUri(const fs::path& path) {
  std::string file_path{path.string()};
  std::string file_extension{path.extension().string()};
  std::ostringstream stream;
  for (std::size_t i = 0; i < file_path.size() - file_extension.size(); ++i) {
    char c = file_path[i];
    if (file_path[i] == '%') {
      char c1 = file_path[++i];
      char c2 = file_path[++i];
      c = static_cast<char>(((c1 - (c1 > '9' ? 'A' - 10 : '0')) << 4) |
                            (c2 - (c2 > '9' ? 'A' - 10 : '0')));
    } else if (file_path[i] == '+') {
      c = ' ';
    }
    stream << c;
  }
  for (auto i : file_extension) {
    stream << ((i >= 'A' && i <= 'Z') ? static_cast<char>(i - 'A' + 'a') : i);
  }
  return stream.str();
}

}  // namespace util
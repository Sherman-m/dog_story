#pragma once

#include <boost/beast.hpp>
#include <filesystem>
#include <fstream>
#include <string>

namespace util {

namespace fs = std::filesystem;
using namespace std::literals;

// Считывает содержимое файла в виде std::string.
std::string LoadContentFromFile(const fs::path& path);

// Вычисляет, является ли base_path вложенным путем в target_path.
bool IsSubPath(fs::path base_path, fs::path target_path);

// Декодирует URL (заменяет hex-числа на символы и приводит расширение
// файла (если оно имеется) в нижний регистр).
// Пример: ./h%65llo%20world.tXT --> ./hello world.txt
std::string DecodeUri(const fs::path& path);

}  // namespace util
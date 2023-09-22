#pragma once

#include <boost/program_options.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace util {

namespace po = boost::program_options;
using namespace std::literals;

// Описывает аргументы командной строки.
struct Args {
  std::string config_file;
  std::string www_root;
  bool randomize_spawn_points = false;
  std::string tick_period;
  std::string state_file;
  std::string save_state_period;
};

// Считывает параметры командой строки
[[nodiscard]] std::optional<Args> ParseCommandLine(int argc,
                                                   const char* const argv[]);

}  //  namespace util
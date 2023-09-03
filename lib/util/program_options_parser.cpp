#include "program_options_parser.h"

namespace util {

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc,
                                                   const char* const argv[]) {
  po::options_description description("All options"s);
  Args args;
  description.add_options()("help,h", "produce help message")(
      "config-file,c", po::value(&args.config_file)->value_name("file"),
      "set config file path")("www-root,w",
                              po::value(&args.www_root)->value_name("dir"),
                              "set static files root")(
      "tick-period,t", po::value(&args.tick_period)->value_name("milliseconds"),
      "set tick period")("randomize-spawn-points",
                         "spawn dogs at random positions");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, description), vm);
  po::notify(vm);

  if (vm.contains("help"s)) {
    std::cout << description;
    return std::nullopt;
  }

  if (!vm.contains("config-file"s)) {
    throw std::runtime_error("Сonfig file has been not specified"s);
  }

  if (!vm.contains("www-root"s)) {
    throw std::runtime_error("www-root has been not specified"s);
  }

  if (vm.contains("randomize-spawn-points"s)) {
    args.randomize_spawn_points = true;
  }
  return args;
}

}  //  namespace util
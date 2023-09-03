#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <thread>

#include "../lib/json_loader/json_loader.h"
#include "../lib/util/program_options_parser.h"
#include "../lib/util/sdk.h"
#include "app/application.h"
#include "http_handler/request_handler.h"
#include "logger/logger.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace json = boost::json;
namespace fs = std::filesystem;

namespace {

// Запускает функцию fn на num_of_workers потоках, включая текущий
template <typename Fn>
void RunWorkers(std::uint32_t num_of_workers, const Fn& fn) {
  num_of_workers = std::max(1u, num_of_workers);
  std::vector<std::jthread> workers;
  workers.reserve(num_of_workers - 1);
  // Запускаем num_of_workers - 1 рабочих потоков, выполняющих функцию fn
  while (--num_of_workers) {
    workers.emplace_back(fn);
  }
  fn();
}

}  // namespace

int main(int argc, const char* argv[]) {
  try {
    // Парсинг параметром командной строки.
    if (auto args = util::ParseCommandLine(argc, argv)) {
      // Инициализация фильтра для логера.
      logger::InitLogFilter();

      const unsigned port = 8080;
      const auto address = net::ip::make_address("0.0.0.0"sv);
      // Загрузка карты из файла и построение модель игры.
      model::Game game = json_loader::LoadGame(args.value().config_file);

      // Инициализация io_context.
      const unsigned num_threads = std::thread::hardware_concurrency();
      net::io_context ioc(static_cast<int>(num_threads));

      // Добавление асинхронного обработчика сигналов SIGINT, SIGTERM и SIGHUP.
      net::signal_set signals(ioc, SIGINT, SIGTERM, SIGHUP);
      signals.async_wait([&ioc](const sys::error_code& ec,
                                [[maybe_unused]] int signal_number) {
        if (ec) {
          throw std::runtime_error("Error signal receiving"s);
        }
        ioc.stop();
      });

      // Инициализация фасада из модуля app.
      std::chrono::milliseconds tick_period = 30ms;
      if (!(args.value().tick_period.empty())) {
        tick_period =
            std::chrono::milliseconds(std::stol(args.value().tick_period));
      }
      auto application = std::make_shared<app::Application>(
          ioc, game, tick_period, !(args.value().tick_period.empty()));
      // Создание обработчика HTTP-запросов и связывание его с моделью игры.
      http_handler::RequestHandler handler(application, args.value().www_root,
                                           args.value().randomize_spawn_points);

      // Запуск обработчика HTTP-запросов, делегируя их обработчику запросов.
      http_server::ServeHttp(ioc, {address, port},
                             [&handler](auto&& req, auto&& send) {
                               handler(std::forward<decltype(req)>(req),
                                       std::forward<decltype(send)>(send));
                             });
      // Логирование о том, что сервер запущен и готов обрабатывать
      // запросы.
      logger::Log(
          json::value{{"port"s, port}, {"address"s, address.to_string()}},
          "server started"sv);

      // Запуск обработки асинхронных операций.
      RunWorkers(std::max(1u, num_threads), [&ioc] { ioc.run(); });
      // Логирование о том, что сервер успешно завершил свою работу.
      logger::Log(json::value{{"code"s, EXIT_SUCCESS}}, "server exited"sv);
    }
    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    // Логирование сообщения об ошибке и завершении работы сервера.
    logger::Log(json::value{{"code"s, EXIT_FAILURE}, {"exception"s, ex.what()}},
                "server exited"sv);
    return EXIT_FAILURE;
  }
}

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <thread>

#include "../lib/json_loader/json_loader.h"
#include "../lib/util/program_options_parser.h"
#include "../lib/util/sdk.h"
#include "app/application.h"
#include "app/ticker.h"
#include "db/database.h"
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

constexpr const char DB_URL_ENV_NAME[]{"GAME_DB_URL"};

// Считывает переменную окружения GAME_DB_URL и вовращает DatabaseConfig.
// Используется при конструировании объекта Application.
auto GetConfigForDatabase(std::uint32_t connection_count) {
  if (const auto* url = std::getenv(DB_URL_ENV_NAME)) {
    return db::DatabaseConfig(connection_count, [url]() {
      return std::make_shared<pqxx::connection>(url);
    });
  }
  throw std::runtime_error(DB_URL_ENV_NAME +
                           " environment variable not found"s);
}

}  // namespace

int main(int argc, const char* argv[]) {
  using Milliseconds = std::chrono::milliseconds;

  try {
    // Парсинг параметром командной строки.
    if (auto args = util::ParseCommandLine(argc, argv)) {
      // Инициализация фильтра для логера.
      logger::InitLogFilter();

      // Указание порта и ip-адреса, через которые сервер будет слушать запросы.
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

      // Установление параметра --state-file <path-to-file>.
      // --state-file <path-to-file> задает путь к файлу, в который приложение
      // должно сохранять свое состояние в процессе работы, а при старте —
      // восстанавливаться.
      //
      //  Сценарии:
      //   - Если параметр не задан, то игровой сервер всегда стартует с
      //     чистого листа и не сохраняет свое состояние в файл;
      //   - Если параметр задан:
      //    - И по указанному пути есть файл, сервер восстанавливает свое
      //      состояние их этого файла;
      //    - Но файла нет, программа начинает все с чистого листа;
      //    - При получении сигналов SIGINT и SIGTERM программа должна
      //      сохранить свое состояние в файл.
      std::string save_file =
          (!args.value().state_file.empty()) ? args.value().state_file : "";
      bool is_save_file_set = !args.value().state_file.empty();

      // Инициализация фасада из модуля app.
      auto application = std::make_shared<app::Application>(
          ioc, game, save_file, is_save_file_set,
          GetConfigForDatabase(num_threads));

      // Установление настроек таймера. Если параметр tick_period задан, то
      // создается объект app::Ticker, который будет отвечать за обновление и
      // сохранения состояния игровых сессий.
      Milliseconds tick_period =
          (!args.value().tick_period.empty())
              ? Milliseconds(std::stol(args.value().tick_period))
              : 30ms;
      bool is_ticker_set = !args.value().tick_period.empty();
      if (is_ticker_set) {
        // Установление параметра --save-state-period <gaming-time-in-ms>.
        // --save-state-period <gaming-time-in-ms> задает период автоматического
        // сохранения состояния сервера.
        //
        //  Сценарии:
        //   - Если параметр не задан, состояние должно сохраняться
        //   автоматически
        //     только перед завершением работы сервера, когда ему направлены
        //     сигналы SIGINT и SIGTERM;
        //   - Если параметр --state-file не задан, то этот параметр
        //     игнорируется.
        Milliseconds save_state_period =
            (!args.value().save_state_period.empty())
                ? Milliseconds(std::stol(args.value().save_state_period))
                : 30ms;
        bool is_save_state_period_set =
            is_save_file_set && !args.value().tick_period.empty();

        auto ticker = std::make_shared<app::Ticker>(
            ioc, tick_period, is_save_state_period_set, save_state_period,
            [application = application->shared_from_this()](
                Milliseconds time_delta) -> bool {
              return application->UpdateAllGameSessions(time_delta);
            },
            [application = application->shared_from_this()]() -> bool {
              return application->SaveGameState();
            });
        ticker->Start();
      }

      // Создание обработчика HTTP-запросов и связывание его с моделью игры.
      http_handler::RequestHandler handler(application, args.value().www_root,
                                           args.value().randomize_spawn_points,
                                           is_ticker_set);

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

      if (is_save_file_set) {
        application->SaveGameState();
      }
    }
    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    // Логирование сообщения об ошибке и завершении работы сервера.
    logger::Log(json::value{{"code"s, EXIT_FAILURE}, {"exception"s, ex.what()}},
                "server exited"sv);
    return EXIT_FAILURE;
  }
}

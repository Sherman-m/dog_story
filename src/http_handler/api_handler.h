#pragma once

#include <any>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <regex>
#include <string>
#include <utility>

#include "../../lib/json_loader/json_loader.h"
#include "../../lib/model/model.h"
#include "../app/application.h"
#include "../app/player.h"
#include "../app/players_table.h"
#include "api_serializer.h"
#include "response_generators.h"

namespace http_handler {

namespace net = boost::asio;
namespace sys = boost::system;
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

using namespace std::literals;

namespace endpoint_storage {

inline constexpr std::string_view kApi = "/api"sv;
inline constexpr std::string_view kApiV1Maps = "/api/v1/maps"sv;
inline constexpr std::string_view kApiV1Map = "/api/v1/maps/"sv;

inline constexpr std::string_view kApiV1GameJoin = "/api/v1/game/join"sv;
inline constexpr std::string_view kApiV1GamePlayers = "/api/v1/game/players"sv;
inline constexpr std::string_view kApiV1GameState = "/api/v1/game/state"sv;
inline constexpr std::string_view kApiV1GamePlayerAction =
    "/api/v1/game/player/action"sv;
inline constexpr std::string_view kApiV1GameTick = "/api/v1/game/tick"sv;

}  // namespace endpoint_storage

namespace common_response_codes {

inline constexpr std::string_view kBadRequest = "badRequest"sv;
inline constexpr std::string_view kNotFound = "notFound"sv;
inline constexpr std::string_view kMapNotFound = "mapNotFound"sv;
inline constexpr std::string_view kInvalidMethod = "invalidMethod"sv;
inline constexpr std::string_view kInvalidArgument = "invalidArgument"sv;
inline constexpr std::string_view kUnknownToken = "unknownToken"sv;
inline constexpr std::string_view kInvalidToken = "invalidToken"sv;
inline constexpr std::string_view kServerError = "serverError"sv;

}  // namespace common_response_codes

// Обрабатывает запросы клиентов к API.
class ApiHandler {
 public:
  using StringRequest = http::request<http::string_body>;
  using StringResponse = http::response<http::string_body>;

  // Заполняет словарь handler_storage_ связками конечных точек с их
  // обработчиками (кроме kApiV1Map, так как эта точка обрабатывается отдельно).
  explicit ApiHandler(std::shared_ptr<app::Application> application,
                      bool randomize_spawn_points);
  ApiHandler(const ApiHandler&) = delete;
  ApiHandler& operator=(const ApiHandler&) = delete;

  // Исходя из значения req.target() находит соответствующий обработчик запроса
  // и вызывает его в случае успеха.
  // Если такой обработчик не найден, проверят req.target() на соответствие
  // конченой точке kApiV1Map и вызывает соответствующий обработчик в случае
  // успеха.
  // Иначе отправляет BadRequest.
  template <typename Send>
  void operator()(StringRequest&& req, Send&& send) {
    std::string target(req.target());
    if (auto it = handler_storage_.find(target); it != handler_storage_.end()) {
      auto handler = std::any_cast<HandlerSignature>(it->second);
      return send(std::move((this->*handler)(std::move(req))));
    } else if (target.starts_with(endpoint_storage::kApiV1Map)) {
      auto handler = std::any_cast<HandlerSignature>(
          handler_storage_[std::string(endpoint_storage::kApiV1Map)]);
      return send(std::move((this->*handler)(std::move(req))));
    }
    return send(std::move(ApiBadRequest(
        ApiSerializer::SerializeError(common_response_codes::kBadRequest,
                                      "Invalid endpoint"sv),
        req.version(), req.keep_alive())));
  }

 private:
  using HandlerStorage = std::unordered_map<std::string, std::any>;
  using HandlerSignature = StringResponse (ApiHandler::*)(StringRequest&&);

  // Проверяет тип токена авторизации игрока и его длину.
  // Если токен авторизации неверен, возвращает ответ со статусом
  // http::status::unauthorized и nullptr вместо указателя на игрока.
  std::pair<StringResponse, const app::Player*> CheckToken(
      const StringRequest& req) const;

  // Проверяет, удовлетворяет ли HTTP-метод запроса ожидаемым HTTP-методам
  // определенного endpoint.
  // В случае ошибки, возвращает ответ со статуcом
  // http::status::method_not_allowed.
  StringResponse CheckHttpMethod(
      http::verb received_method,
      const std::vector<http::verb>& expected_methods,
      std::uint32_t http_version, bool keep_alive) const;

  // Парсят данные запросов в формате JSON.
  // В случае неверных данных возвращают ответ со статусом
  // http::status::bad_request и пустой json::value.
  std::pair<StringResponse, json::value> ParseJoinData(
      const ApiHandler::StringRequest& req, std::uint32_t http_version,
      bool keep_alive) const;
  std::pair<StringResponse, json::value> ParseActionData(
      const StringRequest& req, std::uint32_t http_version,
      bool keep_alive) const;
  std::pair<StringResponse, json::value> ParseTickData(
      const StringRequest& req, std::uint32_t http_version,
      bool keep_alive) const;

  // Находит игровую сессию с id карты равным map_id. Если такой сессии нет,
  // создает новую сессию.
  // В случае неудачного создания сессии возвращает ответ со статусом
  // http::status::internal_server_error.
  std::pair<StringResponse, model::GameSession*> FindGameSession(
      const model::Map::Id& map_id, std::uint32_t http_version,
      bool keep_alive);

  // Обработчики конечных точек.
  StringResponse HandleMapEndpoint(StringRequest&& req);
  StringResponse HandleMapsEndpoint(StringRequest&& req);
  StringResponse HandleJoinEndpoint(StringRequest&& req);
  StringResponse HandlePlayersEndpoint(StringRequest&& req);
  StringResponse HandleStateEndpoint(StringRequest&& req);
  StringResponse HandleActionEndpoint(StringRequest&& req);
  StringResponse HandleTickEndpoint(StringRequest&& req);

  // Функции, отвечающие за формирование ответов для запросов к API.
  StringResponse ApiOkRequest(std::string&& message, std::uint32_t http_version,
                              bool keep_alive) const;
  StringResponse ApiBadRequest(std::string&& message,
                               std::uint32_t http_version,
                               bool keep_alive) const;
  StringResponse ApiNotFound(std::string&& message, std::uint32_t http_version,
                             bool keep_alive) const;
  StringResponse ApiUnauthorized(std::string&& message,
                                 std::uint32_t http_version,
                                 bool keep_alive) const;
  StringResponse ApiMethodNotAllowed(std::string&& message,
                                     std::string_view allowed_methods,
                                     std::uint32_t http_version,
                                     bool keep_alive) const;
  StringResponse ApiInternalServerError(std::string&& message,
                                        std::uint32_t http_version,
                                        bool keep_alive) const;

  std::shared_ptr<app::Application> application_;
  HandlerStorage handler_storage_;
  const unsigned short kAuthTokenMinSize = 7;
  bool randomize_spawn_points_;
};

}  // namespace http_handler

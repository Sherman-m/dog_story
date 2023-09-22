#pragma once

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
inline constexpr std::string_view kApiV1GameRecords = "/api/v1/game/records"sv;
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
                      bool randomize_spawn_points, bool is_ticker_set);
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
    std::string target = ClearTarget(req.target());
    if (auto it = handler_storage_.find(target); it != handler_storage_.end()) {
      auto handler = it->second;
      return send(std::move((this->*handler)(std::move(req))));
    } else if (target.starts_with(endpoint_storage::kApiV1Map)) {
      auto handler = handler_storage_[std::string(endpoint_storage::kApiV1Map)];
      return send(std::move((this->*handler)(std::move(req))));
    }
    return send(std::move(ApiBadRequest(
        ApiSerializer::SerializeError(common_response_codes::kBadRequest,
                                      "Invalid endpoint"sv),
        req.version(), req.keep_alive())));
  }

 private:
  using HandlerPointer = StringResponse (ApiHandler::*)(StringRequest&&);
  using HandlerStorage = std::unordered_map<std::string, HandlerPointer>;

  // Возвращает URL, очищенный от параметров запроса.
  std::string ClearTarget(std::string_view target);

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
      const StringRequest& req, std::uint32_t http_version,
      bool keep_alive) const;
  std::pair<StringResponse, json::value> ParseActionData(
      const StringRequest& req, std::uint32_t http_version,
      bool keep_alive) const;
  std::pair<StringResponse, json::object> ParseRecordsData(
      const StringRequest& req, std::uint32_t http_version,
      bool keep_alive) const;
  std::pair<StringResponse, json::value> ParseTickData(
      const StringRequest& req, std::uint32_t http_version,
      bool keep_alive) const;

  // Находит игровую сессию с id карты равным map_id. Если такой сессии нет,
  // создает новую сессию.
  // В случае неудачного создания сессии возвращает ответ со статусом
  // http::status::internal_server_error.
  std::pair<StringResponse, model::GameSession*> FindFirstGameSessionByMapId(
      const model::Map::Id& map_id, std::uint32_t http_version,
      bool keep_alive);

  // Находит первого попавшегося игрока в игровой сессии с id карты равным
  // map_id и с именем персонажа равным username.
  // В случае неудачи возвращает ответ со статусом http::status::bad_request.
  std::pair<StringResponse, const app::Player*> FindPlayerByMapIdAndUsername(
      const model::Map::Id& map_id, const std::string& username,
      std::uint32_t http_version, bool keep_alive);

  // Обрабатывает конечную точку kApiV1Map для получения информации об
  // определенной карте.
  // Параметры запроса:
  //  - HTTP-методы: GET, HEAD;
  //  - В URI после последнего символа '/' должен быть записан id карты.
  //
  // В случае успеха должен возвращаться ответ, обладающий следующими
  // свойствами:
  //  - Статус-код: 200 OK;
  //  - Content-Type: application/json;
  //  - Content-Length: <body_size>;
  //  - Тело ответа:  JSON-описание карты с указанным id, семантически
  //    эквивалентное представлению карты из конфигурационного файла.
  StringResponse HandleMapEndpoint(StringRequest&& req);

  // Обрабатывает конечную точку kApiV1Maps для получения информации о картах.
  // Параметры запроса:
  //  - HTTP-методы: GET, HEAD;
  //
  // В случае успеха должен возвращаться ответ, обладающий следующими
  // свойствами:
  //  - Статус-код: 200 OK;
  //  - Content-Type: application/json;
  //  - Content-Length: <body_size>;
  //  - Тело ответа: JSON-массив объектов, кратко описывающих карты в игре, с
  //                 полями:
  //    > id - идентификатор карты;
  //    > name - название карты
  StringResponse HandleMapsEndpoint(StringRequest&& req);

  // Обрабатывает конечную точку kApiV1GameJoin для присоединения к игре.
  //
  // Параметры запроса:
  //  - HTTP-методы: POST;
  //  - Headers:
  //    > Content-Type: application/json.
  //  - Тело запроса: JSON-объект с обязательными полями userName и mapId:
  //    имя игрока и id карты. Имя игрока совпадает с именем пса.
  //
  // В случае успеха должен возвращаться ответ, обладающий следующими
  // свойствами:
  //  - Статус-код: 200 OK;
  //  - Content-Type: application/json;
  //  - Content-Length: <body_size>;
  //  - Cache-Control: no-cache;
  //  - Тело ответа: JSON-объект с полями:
  //    > playerId - целое число, задающее id игрока;
  //    > authToken - токен для авторизации в игре - строка, состоящая из
  //    32 случайных шестнадцатеричных цифр.
  StringResponse HandleJoinEndpoint(StringRequest&& req);

  // Обрабатывает конечную точку kApiV1GamePlayers для получения информации об
  // игроках.
  //
  // Параметры запроса:
  //  - HTTP-методы: GET, HEAD;
  //  - Headers:
  //    > Authorization: Bearer <auth_token>.
  //
  // В случае успеха должен возвращаться ответ, обладающий следующими
  // свойствами:
  //  - Статус-код: 200 OK;
  //  - Content-Type: application/json;
  //  - Content-Length: <body_size>;
  //  - Cache-Control: no-cache;
  //  - Тело ответа: JSON-объект. Его ключи - идентификаторы пользователей на
  //    карте. Значение каждого из этих ключей - JSON-объект с единственным
  //    полем name, задающим имя пользователя, под которым он вошёл в
  //    игру.
  StringResponse HandlePlayersEndpoint(StringRequest&& req);

  // Обрабатывает конечную точку kApiV1GameState для получении информации об
  // состоянии игровой сессии, в которой находится игрок.
  //
  // Параметры запроса:
  //  - HTTP-методы: GET, HEAD;
  //  - Headers:
  //    > Authorization: Bearer <auth_token>.
  //
  // В случае успеха должен возвращаться ответ, обладающий следующими
  // свойствами:
  //  - Статус-код: 200 OK;
  //  - Content-Type: application/json;
  //  - Content-Length: <body_size>;
  //  - Cache-Control: no-cache;
  //  - Тело ответа: JSON-объект:
  //    > players - JSON-объект, ключами которого являются идентификаторы
  //                игроков:
  //      > pos - позиция игрока;
  //      > speed - скорость игрока;
  //      > dir - направление игрока;
  //      > bag - JSON-массив, описывающий собранные предметы с полями:
  //        > id - идентификатор потерянного предмета;
  //        > type - тип потерянного предмета.
  //      > score - количество очков, которое набрал игрок.
  //    > lostObjects - JSON-объект, ключами которого являются идентификаторы
  //                    потерянных вещей, которые на данный момент есть в
  //                    игровой сессии и не являются собранными:
  //      > type - тип потерянного предмета;
  //      > pos - позиция потерянного предмета.
  StringResponse HandleStateEndpoint(StringRequest&& req);

  // Обрабатывает конечную точку kApiV1GamePlayerAction для изменения
  // направления игрока.
  //
  // Параметры запроса:
  //  - HTTP-методы: POST;
  //  - Headers:
  //    > Content-Type: application/json;
  //    > Authorization: Bearer <auth_token>.
  //  - Тело запроса: JSON-объект с полем move, которое принимает одно из
  //                  значений:
  //    > "L" - задаёт направление движения персонажа влево (на запад);
  //    > "R" - задаёт направление движения персонажа вправо (на восток);
  //    > "U" - задаёт направление движения персонажа вверх (на север);
  //    > "D" - задаёт направление движения персонажа вниз (на юг);
  //    > "" - останавливает персонажа.
  //
  // В случае успеха должен возвращаться ответ, обладающий следующими
  // свойствами:
  //  - Статус-код: 200 OK;
  //  - Content-Type: application/json;
  //  - Content-Length: <body_size>;
  //  - Cache-Control: no-cache;
  //  - Тело ответа: пустой JSON-объект.
  StringResponse HandleActionEndpoint(StringRequest&& req);

  // Обрабатывает конечную точку kApiV1GameRecords для получения списка
  // рекордсменов.
  //
  // Параметры запроса:
  //  - HTTP-методы: GET, HEAD;
  //  - Query Parameters:
  //     > start - целое число, задающее номер начального элемента
  //               (0 — начальный элемент).
  //     > maxItems — целое число, задающее максимальное количество элементов.
  //                  Если maxItems превышает 100, должна вернуться ошибка с
  //                  кодом 400 Bad Request.
  //
  // В случае успеха должен возвращаться ответ, обладающий следующими
  // свойствами:
  //  - Статус-код: 200 OK;
  //  - Content-Type: application/json;
  //  - Content-Length: <body_size>;
  //  - Cache-Control: no-cache;
  //  - Тело ответа: JSON-массив, состоящий из следующих элементов:
  //    > name - строка, задающая кличку собаки; JSON-объект, ключами которого
  //    являются идентификаторы
  //                игроков:
  //    > score - число, задающее количество очков игрока;
  //    > playTime - время в секундах, которое игрок провёл в игре с момента
  //                 входа до момента выхода из игры.
  StringResponse HandleRecordsEndpoint(StringRequest&& req);

  // Обрабатывает конечную точку kApiV1GameTick для обновления состояния всех
  // игровых сессий.
  //
  // Параметры запроса:
  //  - HTTP-методы: POST;
  //  - Headers:
  //    > Content-Type: application/json;
  //  - Тело запроса: JSON-объект с полем timeDelta, задающим параметр Δt в
  //                  миллисекундах.
  //
  // В случае успеха должен возвращаться ответ, обладающий следующими
  // свойствами:
  //  - Статус-код: 200 OK;
  //  - Content-Type: application/json;
  //  - Content-Length: <body_size>;
  //  - Cache-Control: no-cache;
  //  - Тело ответа: пустой JSON-объект.
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
  const std::uint16_t kAuthTokenMinSize = 7;
  bool randomize_spawn_points_;
  bool is_ticker_set_;
};

}  // namespace http_handler

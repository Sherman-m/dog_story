#include "api_handler.h"

#include <optional>

namespace http_handler {

// Если application->IsTickerSet() == false, то добавляется дополнительный
// обработчик тиков (используется при обращении к /api/v1/game/tick).
ApiHandler::ApiHandler(std::shared_ptr<app::Application> application,
                       bool randomize_spawn_points, bool is_ticker_set)
    : application_(std::move(application)),
      randomize_spawn_points_(randomize_spawn_points),
      is_ticker_set_(is_ticker_set) {
  handler_storage_[std::string(endpoint_storage::kApiV1Map)] =
      &ApiHandler::HandleMapEndpoint;
  handler_storage_[std::string(endpoint_storage::kApiV1Maps)] =
      &ApiHandler::HandleMapsEndpoint;
  handler_storage_[std::string(endpoint_storage::kApiV1GameJoin)] =
      &ApiHandler::HandleJoinEndpoint;
  handler_storage_[std::string(endpoint_storage::kApiV1GamePlayers)] =
      &ApiHandler::HandlePlayersEndpoint;
  handler_storage_[std::string(endpoint_storage::kApiV1GameState)] =
      &ApiHandler::HandleStateEndpoint;
  handler_storage_[std::string(endpoint_storage::kApiV1GamePlayerAction)] =
      &ApiHandler::HandleActionEndpoint;
  handler_storage_[std::string(endpoint_storage::kApiV1GameRecords)] =
      &ApiHandler::HandleRecordsEndpoint;
  if (!is_ticker_set_) {
    handler_storage_[std::string(endpoint_storage::kApiV1GameTick)] =
        &ApiHandler::HandleTickEndpoint;
  }
}

std::string ApiHandler::ClearTarget(std::string_view target) {
  return std::string(target.substr(0, target.rfind('?')));
}

std::pair<ApiHandler::StringResponse, const app::Player*>
ApiHandler::CheckToken(const ApiHandler::StringRequest& req) const {
  StringResponse error_message;
  if (req["Authorization"sv].size() <= kAuthTokenMinSize &&
      !req["Authorization"sv].starts_with("Bearer "sv)) {
    error_message = std::move(ApiUnauthorized(
        ApiSerializer::SerializeError(common_response_codes::kInvalidToken,
                                      "Invalid authorization token"sv),
        req.version(), req.keep_alive()));
    return std::make_pair(error_message, nullptr);
  }
  app::Token token(
      std::move(std::string(req["Authorization"sv].substr(kAuthTokenMinSize))));
  if (auto player = application_->GetPlayerByToken(token)) {
    return std::make_pair(error_message, player);
  }
  error_message = std::move(ApiUnauthorized(
      ApiSerializer::SerializeError(common_response_codes::kUnknownToken,
                                    "Invalid authorization token"sv),
      req.version(), req.keep_alive()));
  return std::make_pair(error_message, nullptr);
}

ApiHandler::StringResponse ApiHandler::CheckHttpMethod(
    http::verb received_method, const std::vector<http::verb>& expected_methods,
    std::uint32_t http_version, bool keep_alive) const {
  std::string expected_methods_str;
  for (auto& method : expected_methods) {
    expected_methods_str += (expected_methods_str.empty())
                                ? http::to_string(method)
                                : ", "s.append(http::to_string(method));
    if (received_method == method) {
      return {};
    }
  }
  return ApiMethodNotAllowed(
      ApiSerializer::SerializeError(
          common_response_codes::kInvalidMethod,
          "Only "s + expected_methods_str + " methods are expected"s),
      expected_methods_str, http_version, keep_alive);
}

std::pair<ApiHandler::StringResponse, json::value> ApiHandler::ParseJoinData(
    const ApiHandler::StringRequest& req, std::uint32_t http_version,
    bool keep_alive) const {
  sys::error_code ec;
  json::value json_request_content = json::parse(req.body(), ec);
  StringResponse error_message;
  if (ec) {
    error_message = std::move(
        ApiBadRequest(ApiSerializer::SerializeError(
                          common_response_codes::kInvalidArgument,
                          "Failed to parse the join the game request JSON"sv),
                      http_version, keep_alive));
    return std::make_pair(error_message, json::value());
  }
  if (auto name = json_request_content.as_object().if_contains("userName"s);
      !name || name->as_string().empty()) {
    error_message = std::move(ApiBadRequest(
        ApiSerializer::SerializeError(common_response_codes::kInvalidArgument,
                                      "Invalid userName"sv),
        http_version, keep_alive));
    return std::make_pair(error_message, json::value());
  }
  if (auto map = json_request_content.as_object().if_contains("mapId"s);
      !map || map->as_string().empty()) {
    error_message = std::move(ApiBadRequest(
        ApiSerializer::SerializeError(common_response_codes::kInvalidArgument,
                                      "Invalid mapId"sv),
        http_version, keep_alive));
    return std::make_pair(error_message, json::value());
  }
  return std::make_pair(error_message, json_request_content);
}

std::pair<ApiHandler::StringResponse, json::value> ApiHandler::ParseActionData(
    const ApiHandler::StringRequest& req, std::uint32_t http_version,
    bool keep_alive) const {
  sys::error_code ec;
  json::value json_request_content = json::parse(req.body(), ec);
  StringResponse error_message;
  if (req["Content-Type"sv].empty() ||
      req["Content-Type"sv] != ContentType::kApplicationJson) {
    error_message = std::move(ApiBadRequest(
        ApiSerializer::SerializeError(common_response_codes::kInvalidArgument,
                                      "Invalid content type"sv),
        http_version, keep_alive));
    return std::make_pair(error_message, json::value());
  }
  if (ec) {
    error_message = std::move(
        ApiBadRequest(ApiSerializer::SerializeError(
                          common_response_codes::kInvalidArgument,
                          "Failed to parse the action request JSON"sv),
                      http_version, keep_alive));
    return std::make_pair(error_message, json::value());
  }
  if (auto movement = json_request_content.as_object().if_contains("move"s);
      !movement ||
      (!std::regex_match(json_loader::JsonObjectToString(*movement),
                         std::regex("[LRUD]"s)) &&
       !movement->as_string().empty())) {
    error_message = std::move(
        ApiBadRequest(ApiSerializer::SerializeError(
                          common_response_codes::kInvalidArgument,
                          "Failed to parse the action request JSON"sv),
                      http_version, keep_alive));
    return std::make_pair(error_message, json::value());
  }
  return std::make_pair(error_message, json_request_content);
}

std::optional<std::string> FindParam(std::string_view target,
                                     const std::string& param) {
  size_t params_start = target.find(param, target.find('?'));
  if (params_start == std::string_view::npos) {
    return std::nullopt;
  }
  size_t begin = target.find('=', params_start) + 1;

  if (begin == std::string_view::npos || begin >= target.size()) {
    return std::nullopt;
  }
  
  size_t end = std::min(target.find('&', begin), target.size());

  return std::string(target.begin() + begin, target.begin() + end);
}

std::pair<ApiHandler::StringResponse, json::object>
ApiHandler::ParseRecordsData(const StringRequest& req,
                             std::uint32_t http_version,
                             bool keep_alive) const {
  StringResponse error_message;
  std::string_view target = req.target();

  json::object request_data;
  auto start_param_opt = FindParam(target, "start"s);
  if (start_param_opt.has_value()) {
    try {
      request_data["start"s] = std::stoll(start_param_opt.value());
    } catch (std::exception& ex) {
        logger::Log(
            json::value{{"code"s, EXIT_FAILURE}, {"exception"s, ex.what()}},
            "error");
        error_message = std::move(ApiBadRequest(
            ApiSerializer::SerializeError(common_response_codes::kInvalidArgument,
                                          "Failed to parse start parameter"sv),
            http_version, keep_alive));
        return std::make_pair(error_message, json::object());
    }
  }

  auto max_items_opt = FindParam(target, "maxItems"s);
  if (max_items_opt.has_value()) {
    try {
      request_data["maxItems"s] = std::stoll(max_items_opt.value());
      if (request_data["maxItems"s].as_int64() > 100) {
        throw std::runtime_error("maxItems greater than 100"s);
      }
    } catch (std::exception& ex) {
        logger::Log(
            json::value{{"code"s, EXIT_FAILURE}, {"exception"s, ex.what()}},
            "error");
        error_message = std::move(ApiBadRequest(
            ApiSerializer::SerializeError(common_response_codes::kInvalidArgument,
                                          "Failed to parse maxItems parameter"sv),
            http_version, keep_alive));
        return std::make_pair(error_message, json::object());
    }
  }

  return std::make_pair(error_message, request_data);
}

std::pair<ApiHandler::StringResponse, json::value> ApiHandler::ParseTickData(
    const ApiHandler::StringRequest& req, std::uint32_t http_version,
    bool keep_alive) const {
  sys::error_code ec;
  json::value json_request_content = json::parse(req.body(), ec);
  StringResponse error_message;
  if (ec) {
    error_message = std::move(ApiBadRequest(
        ApiSerializer::SerializeError(common_response_codes::kInvalidArgument,
                                      "Failed to parse tick request JSON"sv),
        http_version, keep_alive));
    return std::make_pair(error_message, json::value());
  }
  if (auto time_delta =
          json_request_content.as_object().if_contains("timeDelta"s);
      !time_delta || !time_delta->is_number() || !time_delta->is_int64()) {
    error_message = std::move(ApiBadRequest(
        ApiSerializer::SerializeError(common_response_codes::kInvalidArgument,
                                      "Failed to parse tick request JSON"sv),
        http_version, keep_alive));
    return std::make_pair(error_message, json::value());
  }
  return std::make_pair(error_message, std::move(json_request_content));
}

std::pair<ApiHandler::StringResponse, model::GameSession*>
ApiHandler::FindFirstGameSessionByMapId(const model::Map::Id& map_id,
                                        std::uint32_t http_version,
                                        bool keep_alive) {
  StringResponse error_message;
  auto game_session = application_->GetGameSessionByMapId(map_id);
  if (!game_session) {
    game_session = application_->CreateGameSession(map_id);
    if (!game_session) {
      error_message = std::move(
          ApiInternalServerError(ApiSerializer::SerializeError(
                                     common_response_codes::kServerError,
                                     "Failed to create a new game session"sv),
                                 http_version, keep_alive));
      return std::make_pair(error_message, nullptr);
    }
  }
  return std::make_pair(error_message, game_session);
}

std::pair<ApiHandler::StringResponse, const app::Player*>
ApiHandler::FindPlayerByMapIdAndUsername(const model::Map::Id& map_id,
                                         const std::string& username,
                                         std::uint32_t http_version,
                                         bool keep_alive) {
  StringResponse error_message;
  if (auto player =
          application_->GetPlayerByMapIdAndDogName(map_id, username)) {
    return std::make_pair(error_message, player);
  }
  error_message =
      std::move(ApiBadRequest(ApiSerializer::SerializeError(
                                  common_response_codes::kInvalidArgument,
                                  "Failed to find player with this username"sv),
                              http_version, keep_alive));
  return std::make_pair(error_message, nullptr);
}

ApiHandler::StringResponse ApiHandler::HandleMapEndpoint(
    ApiHandler::StringRequest&& req) {
  std::uint32_t http_version = req.version();
  bool keep_alive = req.keep_alive();

  if (auto check_http_method_response = CheckHttpMethod(
          req.method(),
          std::vector<http::verb>{http::verb::get, http::verb::head},
          http_version, keep_alive);
      check_http_method_response.result() == http::status::method_not_allowed) {
    return check_http_method_response;
  }
  std::string map_name(req.target().substr(endpoint_storage::kApiV1Map.size()));
  if (auto map = application_->GetMapById(model::Map::Id(map_name))) {
    return ApiOkRequest(ApiSerializer::SerializeMap(map), http_version,
                        keep_alive);
  }
  return ApiNotFound(
      ApiSerializer::SerializeError(common_response_codes::kMapNotFound,
                                    "Map not found"sv),
      http_version, keep_alive);
}

ApiHandler::StringResponse ApiHandler::HandleMapsEndpoint(
    ApiHandler::StringRequest&& req) {
  std::uint32_t http_version = req.version();
  bool keep_alive = req.keep_alive();

  if (auto check_http_method_response = CheckHttpMethod(
          req.method(),
          std::vector<http::verb>{http::verb::get, http::verb::head},
          http_version, keep_alive);
      check_http_method_response.result() == http::status::method_not_allowed) {
    return check_http_method_response;
  }
  return ApiOkRequest(ApiSerializer::SerializeMaps(application_->GetMaps()),
                      http_version, keep_alive);
}

// В процессе подключения к игре пытается найти такого игрока, который участвует
// в игровой сессии с id карты, равным веденному пользователем map_id, и имя
// которого совпадает с именем персонажа, равным введенному пользователем
// user_name.
// В случае успеха пользователь подключается в качестве найденного игрока.
// В случае неудачи пользователь подключается в качестве нового игрока.
ApiHandler::StringResponse ApiHandler::HandleJoinEndpoint(
    ApiHandler::StringRequest&& req) {
  std::uint32_t http_version = req.version();
  bool keep_alive = req.keep_alive();

  if (auto check_http_method_response = CheckHttpMethod(
          req.method(), std::vector<http::verb>{http::verb::post}, http_version,
          keep_alive);
      check_http_method_response.result() == http::status::method_not_allowed) {
    return check_http_method_response;
  }
  auto parse_user_data_response = ParseJoinData(req, http_version, keep_alive);
  if (parse_user_data_response.first.result() == http::status::bad_request) {
    return parse_user_data_response.first;
  }
  std::string user_name(json_loader::JsonObjectToString(
      parse_user_data_response.second.at("userName"s)));
  model::Map::Id map_id(json_loader::JsonObjectToString(
      parse_user_data_response.second.at("mapId"s)));
  if (auto map = application_->GetMapById(map_id)) {
    if (auto find_player_response = FindPlayerByMapIdAndUsername(
            map_id, user_name, http_version, keep_alive);
        find_player_response.first.result() != http::status::bad_request) {
      return ApiOkRequest(
          ApiSerializer::SerializeJoinResponse(find_player_response.second),
          http_version, keep_alive);
    }
    auto find_game_session_response =
        FindFirstGameSessionByMapId(map_id, http_version, keep_alive);
    if (find_game_session_response.first.result() ==
        http::status::internal_server_error) {
      return find_game_session_response.first;
    }
    if (auto player = application_->JoinToGameSession(
            user_name, find_game_session_response.second->GetId(),
            map->GenerateRandomPosition(randomize_spawn_points_));
        player) {
      return ApiOkRequest(ApiSerializer::SerializeJoinResponse(player),
                          http_version, keep_alive);
    } else {
      return ApiInternalServerError(
          ApiSerializer::SerializeError(common_response_codes::kServerError,
                                        "Failed to join to the game"sv),
          http_version, keep_alive);
    }
  } else {
    return ApiNotFound(
        ApiSerializer::SerializeError(common_response_codes::kMapNotFound,
                                      "Failed to find the map"sv),
        http_version, keep_alive);
  }
}

ApiHandler::StringResponse ApiHandler::HandlePlayersEndpoint(
    ApiHandler::StringRequest&& req) {
  std::uint32_t http_version = req.version();
  bool keep_alive = req.keep_alive();

  if (auto check_http_method_response = CheckHttpMethod(
          req.method(),
          std::vector<http::verb>{http::verb::get, http::verb::head},
          http_version, keep_alive);
      check_http_method_response.result() == http::status::method_not_allowed) {
    return check_http_method_response;
  }
  auto player = CheckToken(req);
  if (player.first.result() == http::status::unauthorized) {
    return player.first;
  }
  if (auto game_session =
          application_->GetGameSessionById(player.second->GetGameSessionId())) {
    auto players =
        application_->GetPlayersByGameSessionId(game_session->GetId());
    return ApiOkRequest(
        ApiSerializer::SerializePlayersInGameSession(game_session, players),
        http_version, keep_alive);
  }
  return ApiNotFound(
      ApiSerializer::SerializeError(common_response_codes::kNotFound,
                                    "Failed to find a game session"sv),
      http_version, keep_alive);
}

ApiHandler::StringResponse ApiHandler::HandleStateEndpoint(
    ApiHandler::StringRequest&& req) {
  std::uint32_t http_version = req.version();
  bool keep_alive = req.keep_alive();

  if (auto check_http_method_response = CheckHttpMethod(
          req.method(),
          std::vector<http::verb>{http::verb::get, http::verb::head},
          http_version, keep_alive);
      check_http_method_response.result() == http::status::method_not_allowed) {
    return check_http_method_response;
  }
  auto player = CheckToken(req);
  if (player.first.result() == http::status::unauthorized) {
    return player.first;
  }
  auto game_session =
      application_->GetGameSessionById(player.second->GetGameSessionId());
  if (game_session) {
    auto players =
        application_->GetPlayersByGameSessionId(game_session->GetId());
    return ApiOkRequest(ApiSerializer::SerializeState(game_session, players),
                        http_version, keep_alive);
  }
  return ApiNotFound(
      ApiSerializer::SerializeError(common_response_codes::kNotFound,
                                    "Failed to find a game session"sv),
      http_version, keep_alive);
}

ApiHandler::StringResponse ApiHandler::HandleActionEndpoint(
    ApiHandler::StringRequest&& req) {
  std::uint32_t http_version = req.version();
  bool keep_alive = req.keep_alive();

  if (auto check_http_method_response = CheckHttpMethod(
          req.method(), std::vector<http::verb>{http::verb::post}, http_version,
          keep_alive);
      check_http_method_response.result() == http::status::method_not_allowed) {
    return check_http_method_response;
  }
  auto player = CheckToken(req);
  if (player.first.result() == http::status::unauthorized) {
    return player.first;
  }
  auto parse_action_data_response =
      ParseActionData(req, http_version, keep_alive);
  if (parse_action_data_response.first.result() == http::status::bad_request) {
    return parse_action_data_response.first;
  }
  std::string movement(json_loader::JsonObjectToString(
      parse_action_data_response.second.at("move"s)));
  if (application_->MovePlayer(player.second->GetGameSessionId(),
                               player.second->GetDogId(), movement)) {
    return ApiOkRequest(json::serialize(json::object()), http_version,
                        keep_alive);
  }
  return ApiInternalServerError(
      ApiSerializer::SerializeError(common_response_codes::kServerError,
                                    "Failed to move player"sv),
      http_version, keep_alive);
}

ApiHandler::StringResponse ApiHandler::HandleRecordsEndpoint(
    StringRequest&& req) {
  std::uint32_t http_version = req.version();
  bool keep_alive = req.keep_alive();

  if (auto check_http_method_response = CheckHttpMethod(
          req.method(),
          std::vector<http::verb>{http::verb::get, http::verb::head},
          http_version, keep_alive);
      check_http_method_response.result() == http::status::method_not_allowed) {
    return check_http_method_response;
  }
  auto parse_records_data_response =
      ParseRecordsData(req, http_version, keep_alive);
  if (parse_records_data_response.first.result() == http::status::bad_request) {
    return parse_records_data_response.first;
  }
  const auto start_param =
      parse_records_data_response.second.contains("start"s)
          ? parse_records_data_response.second.at("start"s).as_int64()
          : 0;
  const auto max_items_param =
      parse_records_data_response.second.contains("maxItems"s)
          ? parse_records_data_response.second.at("maxItems"s).as_int64()
          : 50;

  auto retired_dogs =
      application_->GetRetiredPlayers(start_param, max_items_param);
  return ApiOkRequest(ApiSerializer::SerializeRecordsResponse(retired_dogs),
                      http_version, keep_alive);
}

ApiHandler::StringResponse ApiHandler::HandleTickEndpoint(
    ApiHandler::StringRequest&& req) {
  std::uint32_t http_version = req.version();
  bool keep_alive = req.keep_alive();

  if (auto check_http_method_response = CheckHttpMethod(
          req.method(), std::vector<http::verb>{http::verb::post}, http_version,
          keep_alive);
      check_http_method_response.result() == http::status::method_not_allowed) {
    return check_http_method_response;
  }
  auto parse_tick_data_response = ParseTickData(req, http_version, keep_alive);
  if (parse_tick_data_response.first.result() == http::status::bad_request) {
    return parse_tick_data_response.first;
  }
  std::chrono::milliseconds time_delta(
      parse_tick_data_response.second.at("timeDelta"s).as_int64());
  if (application_->UpdateAllGameSessions(time_delta) &&
      application_->SaveGameState()) {
    return ApiOkRequest(json::serialize(json::object()), http_version,
                        keep_alive);
  }
  return ApiInternalServerError(
      ApiSerializer::SerializeError(common_response_codes::kServerError,
                                    "Failed to update game state"sv),
      http_version, keep_alive);
}

ApiHandler::StringResponse ApiHandler::ApiOkRequest(std::string&& message,
                                                    std::uint32_t http_version,
                                                    bool keep_alive) const {
  return OkRequest<http::string_body>(std::move(message), http_version,
                                      keep_alive,
                                      ContentType::kApplicationJson);
}

ApiHandler::StringResponse ApiHandler::ApiBadRequest(std::string&& message,
                                                     std::uint32_t http_version,
                                                     bool keep_alive) const {
  return BadRequest<http::string_body>(std::move(message), http_version,
                                       keep_alive,
                                       ContentType::kApplicationJson);
}

ApiHandler::StringResponse ApiHandler::ApiNotFound(std::string&& message,
                                                   std::uint32_t http_version,
                                                   bool keep_alive) const {
  return NotFound<http::string_body>(std::move(message), http_version,
                                     keep_alive, ContentType::kApplicationJson);
}

ApiHandler::StringResponse ApiHandler::ApiUnauthorized(
    std::string&& message, std::uint32_t http_version, bool keep_alive) const {
  return Unauthorized<http::string_body>(std::move(message), http_version,
                                         keep_alive,
                                         ContentType::kApplicationJson);
}

ApiHandler::StringResponse ApiHandler::ApiMethodNotAllowed(
    std::string&& message, std::string_view allowed_methods,
    std::uint32_t http_version, bool keep_alive) const {
  return MethodNotAllowed<http::string_body>(
      std::move(message), allowed_methods, http_version, keep_alive,
      ContentType::kApplicationJson);
}

ApiHandler::StringResponse ApiHandler::ApiInternalServerError(
    std::string&& message, std::uint32_t http_version, bool keep_alive) const {
  return InternalServerError<http::string_body>(std::move(message),
                                                http_version, keep_alive,
                                                ContentType::kApplicationJson);
}

}  // namespace http_handler

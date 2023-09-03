#include "api_handler.h"

namespace http_handler {

// Если application->IsTickerSet() == false, то добавляется дополнительный
// обработчик тиков (используется при обращении к /api/v1/game/tick).
ApiHandler::ApiHandler(std::shared_ptr<app::Application> application,
                       bool randomize_spawn_points)
    : application_(std::move(application)),
      randomize_spawn_points_(randomize_spawn_points) {
  handler_storage_[std::string(endpoint_storage::kApiV1Map)] =
      std::make_any<HandlerSignature>(&ApiHandler::HandleMapEndpoint);
  handler_storage_[std::string(endpoint_storage::kApiV1Maps)] =
      std::make_any<HandlerSignature>(&ApiHandler::HandleMapsEndpoint);
  handler_storage_[std::string(endpoint_storage::kApiV1GameJoin)] =
      std::make_any<HandlerSignature>(&ApiHandler::HandleJoinEndpoint);
  handler_storage_[std::string(endpoint_storage::kApiV1GamePlayers)] =
      std::make_any<HandlerSignature>(&ApiHandler::HandlePlayersEndpoint);
  handler_storage_[std::string(endpoint_storage::kApiV1GameState)] =
      std::make_any<HandlerSignature>(&ApiHandler::HandleStateEndpoint);
  handler_storage_[std::string(endpoint_storage::kApiV1GamePlayerAction)] =
      std::make_any<HandlerSignature>(&ApiHandler::HandleActionEndpoint);

  if (!application_->IsTickerSet()) {
    handler_storage_[std::string(endpoint_storage::kApiV1GameTick)] =
        std::make_any<HandlerSignature>(&ApiHandler::HandleTickEndpoint);
  }
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
                          "Failed to parse the request to join the game"sv),
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
                          "Failed to parse the request to join the game"sv),
                      http_version, keep_alive));
    return std::make_pair(error_message, json::value());
  }
  if (auto movement = json_request_content.as_object().if_contains("move"s);
      !movement ||
      (!std::regex_match(json_loader::JsonObjectToString(*movement),
                         std::regex("[LRUD]"s)) &&
       !movement->as_string().empty())) {
    error_message = std::move(ApiBadRequest(
        ApiSerializer::SerializeError(common_response_codes::kInvalidArgument,
                                      "Failed to parse action"sv),
        http_version, keep_alive));
    return std::make_pair(error_message, json::value());
  }
  return std::make_pair(error_message, json_request_content);
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
ApiHandler::FindGameSession(const model::Map::Id& map_id,
                            std::uint32_t http_version, bool keep_alive) {
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
  return ApiOkRequest(ApiSerializer::SerializeMaps(application_->GetMaps()),
                      req.version(), req.keep_alive());
}

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
    auto find_game_session_response =
        FindGameSession(map_id, http_version, keep_alive);
    if (find_game_session_response.first.result() ==
        http::status::internal_server_error) {
      return find_game_session_response.first;
    }
    if (auto join_response = application_->JoinToGameSession(
            user_name, find_game_session_response.second->GetId(),
            map->GenerateRandomPosition(randomize_spawn_points_));
        join_response.second) {
      return ApiOkRequest(ApiSerializer::SerializeJoinResponse(join_response),
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
    auto players = application_->GetPlayersInSession(game_session->GetId());
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
    auto players = application_->GetPlayersInSession(game_session->GetId());
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
  if (application_->UpdateAllGameSessions(time_delta)) {
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
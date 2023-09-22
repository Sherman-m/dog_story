#include "request_handler.h"

namespace http_handler {

RequestHandler::RequestHandler(std::shared_ptr<app::Application> application,
                               std::string www_root_path,
                               bool randomize_spawn_points, bool ticker_is_set)
    : api_handler_(std::move(application), randomize_spawn_points,
                   ticker_is_set),
      www_root_path_(std::move(www_root_path)) {}

}  // namespace http_handler
#pragma once

#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>
#include <string>
#include <utility>

#include "../../lib/model/model.h"
#include "../../lib/util/file_handler.h"
#include "../http_server/http_server.h"
#include "api_handler.h"
#include "response_generators.h"

// Содержит код, отвечающий за обработку HTTP-запросов клиентов
namespace http_handler {

namespace beast = boost::beast;
namespace sys = boost::system;
namespace http = beast::http;
namespace fs = std::filesystem;

// Отвечает за обработку запросов.
// При обращении к API делегирует обработку запросов ApiHandler.
class RequestHandler {
 public:
  explicit RequestHandler(std::shared_ptr<app::Application> application,
                          std::string www_root_path,
                          bool randomize_spawn_points, bool ticker_is_set);

  RequestHandler(const RequestHandler&) = delete;
  RequestHandler& operator=(const RequestHandler&) = delete;

  template <typename Body, typename Allocator, typename Send>
  void operator()(http::request<Body, http::basic_fields<Allocator>>&& req,
                  Send&& send) {
    if (req.target().starts_with(endpoint_storage::kApi)) {
      return api_handler_(std::forward<decltype(req)>(req),
                          std::forward<Send>(send));
    }
    return SendStaticData(std::forward<decltype(req)>(req),
                          std::forward<Send>(send));
  }

 private:
  // SendStaticData отправляет статические файлы: .html, .js, .css, ...
  template <typename Body, typename Allocator, typename Send>
  void SendStaticData(http::request<Body, http::basic_fields<Allocator>>&& req,
                      Send&& send) {
    std::string decoded_uri = (req.target().size() > 1)
                                  ? util::DecodeUri(req.target())
                                  : "/index.html"s;
    fs::path path_to_file(www_root_path_ + decoded_uri);
    if (!util::IsSubPath(www_root_path_, path_to_file)) {
      send(std::move(BadRequest<http::string_body>(
          "Bad request: Incorrect path"s, req.version(), req.keep_alive(),
          ContentType::KTextPlain)));
    } else if (!fs::exists(path_to_file) || fs::is_directory(path_to_file)) {
      send(std::move(NotFound<http::string_body>(
          "Not found: File "s.append(path_to_file.filename()) + " not found"s,
          req.version(), req.keep_alive(), ContentType::KTextPlain)));
    } else {
      http::file_body::value_type file;
      if (sys::error_code ec;
          file.open(path_to_file.c_str(), beast::file_mode::read, ec), ec) {
        send(std::move(BadRequest<http::string_body>(
            "Bad request: Could not open "s.append(path_to_file.filename()),
            req.version(), req.keep_alive(), ContentType::KTextPlain)));
      } else {
        send(std::move(OkRequest<http::file_body>(
            std::move(file), req.version(), req.keep_alive(),
            ContentType::ConvertExtensionToMimeType(
                path_to_file.extension().string()))));
      }
    }
  }

  ApiHandler api_handler_;
  std::string www_root_path_;
};

}  // namespace http_handler

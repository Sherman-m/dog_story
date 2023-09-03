#pragma once

#include <boost/beast.hpp>
#include <string>

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;

using namespace std::literals;

// Содержит в себе константы для заголовка Content-Type запроса.
// ConvertExtensionToMimeType выбирает нужный Content-Type исходя из расширения
// файла.
class ContentType {
 public:
  ContentType() = delete;
  ContentType(ContentType&) = delete;
  ContentType& operator=(ContentType&) = delete;
  ContentType(ContentType&&) = delete;
  ContentType& operator=(ContentType&&) = delete;

  static std::string_view ConvertExtensionToMimeType(
      std::string_view extension) noexcept;

  static constexpr std::string_view kTextHtml = "text/html"sv;
  static constexpr std::string_view kTextCss = "text/css"sv;
  static constexpr std::string_view KTextPlain = "text/plain"sv;
  static constexpr std::string_view kTextJavascript = "text/js"sv;
  static constexpr std::string_view kApplicationJson = "application/json"sv;
  static constexpr std::string_view kApplicationXml = "application/xml"sv;
  static constexpr std::string_view kApplicationOctetStream =
      "application/octet-stream"sv;
  static constexpr std::string_view kImagePng = "image/png"sv;
  static constexpr std::string_view kImageJpeg = "image/jpeg"sv;
  static constexpr std::string_view kImageGif = "image/gif"sv;
  static constexpr std::string_view kImageBmp = "image/bmp"sv;
  static constexpr std::string_view kImageVndMicrosoftIcon =
      "image/vnd.microsoft.icon"sv;
  static constexpr std::string_view kImageTiff = "image/tiff"sv;
  static constexpr std::string_view kImageSvgXml = "image/svg+xml"sv;
  static constexpr std::string_view kAudioMpeg = "audio/mpeg"sv;

 private:
  using Extension = std::string_view;
  using MimeType = std::string_view;
  inline static std::unordered_map<Extension, MimeType> mime_types_{
      {".htm"sv, kTextHtml},
      {".html"sv, kTextHtml},
      {".css"sv, kTextCss},
      {".txt"sv, KTextPlain},
      {".js"sv, kTextJavascript},
      {".json"sv, kApplicationJson},
      {".xml"sv, kApplicationXml},
      {""sv, kApplicationOctetStream},
      {".png"sv, kImagePng},
      {".jpg"sv, kImageJpeg},
      {".jpe"sv, kImageJpeg},
      {".jpeg"sv, kImageJpeg},
      {".gif"sv, kImageGif},
      {".bmp"sv, kImageBmp},
      {".ico"sv, kImageVndMicrosoftIcon},
      {".tiff"sv, kImageTiff},
      {".tif"sv, kImageTiff},
      {".svg"sv, kImageSvgXml},
      {".svgz"sv, kImageSvgXml},
      {".mp3"sv, kAudioMpeg}};
};

// Является общей функцией для создания http::response.
template <typename Body, typename ValueType>
http::response<Body> MakeResponse(http::status status, ValueType&& message,
                                  std::uint32_t http_version, bool keep_alive,
                                  std::string_view content_type) {
  http::response<Body> response(status, http_version);
  response.set(http::field::content_type, content_type);
  response.body() = std::forward<ValueType>(message);
  response.keep_alive(keep_alive);
  response.prepare_payload();
  response.set(http::field::cache_control, "no-cache"sv);
  return response;
}

// Функции OkRequest, BadRequest, NotFound, Unauthorized, MethodNotAllowed,
// InternalServerError отвечают за формирование ответов.
// Благодаря шаблонному параметру позволяют формировать ответы как для
// запросов REST kApi, так и для запросов статических файлов.
template <typename Body, typename ValueType>
http::response<Body> OkRequest(ValueType&& message, std::uint32_t http_version,
                               bool keep_alive, std::string_view content_type) {
  return MakeResponse<Body>(http::status::ok, std::forward<ValueType>(message),
                            http_version, keep_alive, content_type);
}

template <typename Body, typename ValueType>
http::response<Body> BadRequest(ValueType&& message, std::uint32_t http_version,
                                bool keep_alive,
                                std::string_view content_type) {
  return MakeResponse<Body>(http::status::bad_request,
                            std::forward<ValueType>(message), http_version,
                            keep_alive, content_type);
}

template <typename Body, typename ValueType>
http::response<Body> NotFound(ValueType&& message, std::uint32_t http_version,
                              bool keep_alive, std::string_view content_type) {
  return MakeResponse<Body>(http::status::not_found,
                            std::forward<ValueType>(message), http_version,
                            keep_alive, content_type);
}

template <typename Body, typename ValueType>
http::response<Body> Unauthorized(ValueType&& message,
                                  std::uint32_t http_version, bool keep_alive,
                                  std::string_view content_type) {
  return MakeResponse<Body>(http::status::unauthorized,
                            std::forward<ValueType>(message), http_version,
                            keep_alive, content_type);
}

template <typename Body, typename ValueType>
http::response<Body> MethodNotAllowed(ValueType&& message,
                                      std::string_view allowed_methods,
                                      std::uint32_t http_version,
                                      bool keep_alive,
                                      std::string_view content_type) {
  auto response = MakeResponse<Body>(http::status::method_not_allowed,
                                     std::forward<ValueType>(message),
                                     http_version, keep_alive, content_type);
  response.set(http::field::allow, allowed_methods);
  return response;
}

template <typename Body, typename ValueType>
http::response<Body> InternalServerError(ValueType&& message,
                                         std::uint32_t http_version,
                                         bool keep_alive,
                                         std::string_view content_type) {
  return MakeResponse<Body>(http::status::unauthorized,
                            std::forward<ValueType>(message), http_version,
                            keep_alive, content_type);
}

}  // namespace http_handler
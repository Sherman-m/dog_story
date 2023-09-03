#include "http_server.h"

namespace http_server {

void LogError(beast::error_code ec, std::string_view where) {
  logger::Log(
      json::value{
          {"code"s, ec.value()}, {"text"s, ec.message()}, {"where"s, where}},
      "error"sv);
}

SessionBase::SessionBase(tcp::socket&& socket) : stream_(std::move(socket)) {}

void SessionBase::Run() {
  net::dispatch(stream_.get_executor(),
                beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
}

void SessionBase::Read() {
  request_.clear();
  request_.body().clear();
  stream_.expires_after(30s);
  http::async_read(
      stream_, buffer_, request_,
      beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
}

// Логирует получаение запроса
void SessionBase::OnRead(beast::error_code ec,
                         [[maybe_unused]] std::size_t bytes_read) {
  if (ec == http::error::end_of_stream) {
    return Close();
  }
  if (ec) {
    return LogError(ec, "read"sv);
  }
  logger::Log(
      json::value{
          {"ip"s, stream_.socket().remote_endpoint().address().to_string()},
          {"URI"s, request_.target()},
          {"method"s, request_.method_string()}},
      "request received"sv);
  HandleRequest(std::move(request_));
}

void SessionBase::Close() {
  beast::error_code ec;
  stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
  if (ec) {
    return LogError(ec, "close"sv);
  }
}

void SessionBase::OnWrite(bool close, beast::error_code ec,
                          [[maybe_unused]] std::size_t bytes_written) {
  if (ec) {
    return LogError(ec, "write"sv);
  }
  if (close) {
    return Close();
  }
  Read();
}

}  // namespace http_server

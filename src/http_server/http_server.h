#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <boost/thread.hpp>
#include <chrono>

#include "../../lib/util/sdk.h"
#include "../logger/logger.h"

// Содержит ядро асинхронного сервера
namespace http_server {

namespace net = boost::asio;
namespace sys = boost::system;
using tcp = net::ip::tcp;
namespace beast = boost::beast;
namespace http = beast::http;

namespace json = boost::json;

using namespace std::literals;

// Логирует сетевые ошибки.
void LogError(beast::error_code ec, std::string_view where);

// Является каркасом для класса Session.
// Создан для уменьшения машинного кода и содержит только нешаблонные параметры.
class SessionBase {
 public:
  using Milliseconds = std::chrono::milliseconds;
  using Clock = std::chrono::steady_clock;

  SessionBase(const SessionBase&) = delete;
  SessionBase& operator=(const SessionBase&) = delete;

  void Run();

 protected:
  using HttpRequest = http::request<http::string_body>;

  explicit SessionBase(tcp::socket&& socket);
  virtual ~SessionBase() = default;

  // Отправляет клиенту ответ. Логирует об окончании формирования ответа.
  template <typename Body, typename Fields>
  void Write(http::response<Body, Fields>&& response,
             const Clock::time_point response_start) {
    Clock::time_point response_end = Clock::now();
    logger::Log(
        json::value{
            {"response_time"s, std::chrono::duration_cast<Milliseconds>(
                                   response_end - response_start)
                                   .count()},
            {"code"s, response.result_int()},
            {"content_type"s, (response[http::field::content_type] != ""sv)
                                  ? response[http::field::content_type]
                                  : "null"s}},
        "response sent"sv);

    auto safe_response =
        std::make_shared<http::response<Body, Fields>>(std::move(response));
    auto self = GetSharedThis();
    http::async_write(
        stream_, *safe_response,
        [safe_response, self](beast::error_code ec, std::size_t bytes_written) {
          self->OnWrite(safe_response->need_eof(), ec, bytes_written);
        });
  }

 private:
  void Read();
  void OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read);
  void Close();
  void OnWrite(bool close, beast::error_code ec,
               [[maybe_unused]] std::size_t bytes_written);

  virtual void HandleRequest(HttpRequest&& request) = 0;
  virtual std::shared_ptr<SessionBase> GetSharedThis() = 0;

  beast::tcp_stream stream_;
  beast::flat_buffer buffer_;
  HttpRequest request_;
};

// Наследует класс SessionBase и содержит в себе только шаблонный параметр
// RequestHandler.
// Отвечает за шаги обработки HTTP-сессии:
//  - Чтение запроса;
//  - Обработка запроса;
//  - Отправка ответа.
template <typename RequestHandler>
class Session : public SessionBase,
                public std::enable_shared_from_this<Session<RequestHandler>> {
 public:
  template <typename Handler>
  Session(tcp::socket&& socket, Handler&& request_handler)
      : SessionBase(std::move(socket)),
        request_handler_(std::forward<Handler>(request_handler)) {}

 private:
  std::shared_ptr<SessionBase> GetSharedThis() override {
    return this->shared_from_this();
  }

  // Устанавливает время начала формирования ответа и обрабатывает запрос
  // клиента.
  void HandleRequest(HttpRequest&& request) override {
    Clock::time_point start = Clock::now();
    request_handler_(std::move(request),
                     [start, self = this->shared_from_this()](auto&& response) {
                       self->Write(std::forward<decltype(response)>(response),
                                   start);
                     });
  }

  RequestHandler request_handler_;
};

// Асинхронно принимает TCP-соединения клиентов с сервером. Приняв соединение,
// Listener создает экземпляр класса Session, который отвечает за обработку
// HTTP-сессии.
template <typename RequestHandler>
class Listener : public std::enable_shared_from_this<Listener<RequestHandler>> {
 public:
  template <typename Handler>
  explicit Listener(net::io_context& ioc, const tcp::endpoint& endpoint,
                    Handler&& request_handler)
      : ioc_(ioc),
        acceptor_(net::make_strand(ioc)),
        request_handler_(std::forward<Handler>(request_handler)) {
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(net::socket_base::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen(net::socket_base::max_listen_connections);
  }

  void Run() { DoAccept(); }

 private:
  void DoAccept() {
    acceptor_.async_accept(net::make_strand(ioc_),
                           beast::bind_front_handler(&Listener::OnAccept,
                                                     this->shared_from_this()));
  }

  void OnAccept(sys::error_code ec, tcp::socket socket) {
    if (ec) {
      return LogError(ec, "accept"sv);
    }
    AsyncRunSession(std::move(socket));
    DoAccept();
  }

  void AsyncRunSession(tcp::socket&& socket) {
    std::make_shared<Session<RequestHandler>>(std::move(socket),
                                              request_handler_)
        ->Run();
  }

  net::io_context& ioc_;
  tcp::acceptor acceptor_;
  RequestHandler request_handler_;
};

// Создает экземпляры класса Listener для приема соединения от клиентов и
// асинхронной обработки запросов.
template <typename RequestHandler>
void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint,
               RequestHandler&& handler) {
  using MyListener = Listener<std::decay_t<RequestHandler>>;

  std::make_shared<MyListener>(ioc, endpoint,
                               std::forward<RequestHandler>(handler))
      ->Run();
}

}  // namespace http_server

#include "logger.h"

namespace logger {

void Formatter(const logging::record_view& rec,
               logging::formatting_ostream& log_stream) {
  json::object json_log;

  auto ts = rec[timestamp];
  json_log["timestamp"s] = boost::posix_time::to_iso_extended_string(*ts);
  json_log["data"s] = *rec[additional_data];
  json_log["message"s] = *rec[expr::smessage];

  log_stream << json::serialize(json_log);
}

// Настраивает фильтры при логировании в консоль.
void InitLogFilter() {
  logging::add_common_attributes();

  logging::add_console_log(std::cout, keywords::format = &Formatter,
                           keywords::auto_flush = true);
}

void Log(const json::value& data, const std::string_view& message) {
  BOOST_LOG_TRIVIAL(info) << logging::add_value("AdditionalData"s, data)
                          << message;
}

}  // namespace logger
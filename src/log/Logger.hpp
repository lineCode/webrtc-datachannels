#pragma once

#include <memory>
#include <string>

// IWYU pragma: begin_exports
#include <g3log/g3log.hpp>
#include <g3log/logcapture.hpp>
#include <g3log/loglevels.hpp>
#include <g3log/logmessage.hpp>
#include <g3log/logworker.hpp>
#include <g3log/sinkhandle.hpp>
// IWYU pragma: end_exports

namespace gloer {
namespace log {

struct CustomConsoleSink;

/**
 * Supported log levels: DEBUG, INFO, WARNING, FATAL
 * @Usage
 * LOG(WARNING) << "This log " << "call";
 **/
class Logger {
public:
  Logger();

  ~Logger() {
    // NOTE: no need to call g3::internal::shutDownLogging();
    // 1 Shutdownlogging does stop the logging, sets the logging ptr to null
    // 2 G3log should be stopped with RAII or with stop hooks
  }

  static Logger& instance();

  std::shared_ptr<::g3::LogWorker> getLogWorker() const;

private:
  std::shared_ptr<::g3::LogWorker> logWorker_{::g3::LogWorker::createLogWorker()};
  std::unique_ptr<::g3::FileSinkHandle> fileSinkHandle_;
  std::unique_ptr<::g3::SinkHandle<CustomConsoleSink>> consoleSinkHandle_;
  bool enableConsoleSink_ = true;
  bool enableFileSink_ = true;
  std::string log_prefix_ = "wrtcServer";
  std::string log_directory_;
  std::string log_default_id_ = "";
};

} // namespace log
} // namespace gloer

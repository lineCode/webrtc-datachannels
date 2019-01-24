#include "log/Logger.hpp" // IWYU pragma: associated
#include "config/ServerConfig.hpp"
#include "rang.hpp"
#include "storage/path.hpp"
#include <filesystem>
#include <iostream>

namespace gloer {
namespace log {

namespace fs = std::filesystem;

struct CustomConsoleSink {
  rang::fg getLevelColor(const LEVELS level) const {
    if (level.value == WARNING.value) {
      return rang::fg::yellow;
    }

    if (level.value == DEBUG.value) {
      return rang::fg::green;
    }

    if (g3::internal::wasFatal(level)) {
      return rang::fg::red;
    }

    return rang::fg::reset;
  }

  void ReceiveLogMessage(g3::LogMessageMover logEntry) {
    auto logLevel = logEntry.get()._level;
    auto color = getLevelColor(logLevel);

    std::cout << color << logEntry.get().toString() << rang::fg::reset << rang::bg::reset << "\n";
  }
};

Logger::Logger() {
  const fs::path workdir = gloer::storage::getThisBinaryDirectoryPath();
  log_directory_ = (workdir / gloer::config::ASSETS_DIR).string();
  if (enableFileSink_) {
    consoleSinkHandle_ = logWorker_->addSink(std::make_unique<CustomConsoleSink>(),
                                             &CustomConsoleSink::ReceiveLogMessage);
  }

  if (enableConsoleSink_) {
    fileSinkHandle_ = logWorker_->addDefaultLogger(log_prefix_, log_directory_, log_default_id_);
  }
  g3::initializeLogging(logWorker_.get());
}

Logger& Logger::instance() {
  static Logger instance;
  return instance;
}

std::shared_ptr<::g3::LogWorker> Logger::getLogWorker() const { return logWorker_; }

} // namespace log
} // namespace gloer

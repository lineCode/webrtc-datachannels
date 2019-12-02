#include "log/Logger.hpp" // IWYU pragma: associated
#include "config/ServerConfig.hpp"
#include "rang.hpp"
#include "storage/path.hpp"

#ifndef __has_include
  static_assert(false, "__has_include not supported");
#else
#  if __has_include(<filesystem>)
#    include <filesystem>
     namespace fs = std::filesystem;
#  elif __has_include(<experimental/filesystem>)
#    include <experimental/filesystem>
     namespace fs = std::experimental::filesystem;
#  elif __has_include(<boost/filesystem.hpp>)
#    include <boost/filesystem.hpp>
     namespace fs = boost::filesystem;
#  endif
#endif

#include <iostream>

namespace gloer {
namespace log {

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

Logger::Logger() { initLogging(); }

Logger::Logger(bool enableConsoleSink, bool enableFileSink)
    : enableConsoleSink_(enableConsoleSink), enableFileSink_(enableFileSink) {
  initLogging();
}

Logger::~Logger() { shutDownLogging(); }

void Logger::initLogging() {
  const ::fs::path workdir = gloer::storage::getThisBinaryDirectoryPath();
  log_directory_ = (workdir / gloer::config::ASSETS_DIR).string();

  if (!logWorker_ || !logWorker_.get()) {
    LOG(WARNING) << "Logger: invalid logWorker_";
  }

  if (enableConsoleSink_) {
    consoleSinkHandle_ = logWorker_->addSink(std::make_unique<CustomConsoleSink>(),
                                             &CustomConsoleSink::ReceiveLogMessage);
  }

  if (enableFileSink_) {
    fileSinkHandle_ = logWorker_->addDefaultLogger(log_prefix_, log_directory_, log_default_id_);
  }
  g3::initializeLogging(logWorker_.get());
}

void Logger::shutDownLogging() {
  // NOTE: no need to call g3::internal::shutDownLogging();
  // 1 Shutdownlogging does stop the logging, sets the logging ptr to null
  // 2 G3log should be stopped with RAII or with stop hooks
  if (logWorker_.get())
    logWorker_ = nullptr;
  if (consoleSinkHandle_.get())
    consoleSinkHandle_ = nullptr;
  if (fileSinkHandle_.get())
    fileSinkHandle_ = nullptr;
}

std::shared_ptr<::g3::LogWorker> Logger::getLogWorker() const { return logWorker_; }

} // namespace log
} // namespace gloer

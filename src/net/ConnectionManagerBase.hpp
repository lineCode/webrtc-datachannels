#pragma once

#include <mutex>
#include <string>

namespace gloer {
namespace config {
struct ServerConfig;
} // namespace config
} // namespace gloer

namespace gloer {
namespace net {

template<typename SessionType>
class ConnectionManagerBase {
public:
  ConnectionManagerBase() {}

  virtual ~ConnectionManagerBase() {}

  /**
   * @example:
   * std::time_t t = std::chrono::system_clock::to_time_t(p);
   * std::string msg = "server_time: ";
   * msg += std::ctime(&t);
   * sm->sendToAll(msg);
   **/
  virtual void sendToAll(const std::string& message) = 0;

  virtual void sendTo(const SessionType& sessionID, const std::string& message) = 0;

  virtual void run(const gloer::config::ServerConfig& serverConfig) = 0;

  virtual void finish() = 0;
};

} // namespace net
} // namespace gloer

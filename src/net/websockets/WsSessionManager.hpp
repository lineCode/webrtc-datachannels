#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace utils {
namespace net {
class WsSession;
} // namespace net
} // namespace utils

namespace utils {
namespace net {

/**
 * @brief manages currently valid sessions
 */
class WsSessionManager {
public:
  void registerSession(const std::shared_ptr<WsSession>& session);
  void unregisterSession(std::string id);
  void interpret(size_t id, const std::string& message);
  void sendToAll(const std::string message);
  void handleAllPlayerMessages();
  void doToAllPlayers(std::function<void(std::shared_ptr<WsSession>)> func);
  /**
   * @brief returns the number of connected clients
   *
   * @return number of valid sessions
   */
  size_t getSessionsCount() const { return sessions_.size(); }
  size_t getSessions() const { return sessions_.size(); }
  // uint32_t getMaxSessionId() const { return maxSessionId_; }
  // TODO: limit max num of open sessions
  // uint32_t maxSessionId_ = 0;
  // TODO: limit max num of open connections per IP
  // uint32_t maxConnectionsPerIP_ = 0;

private:
  std::unordered_map<std::string, std::shared_ptr<WsSession>> sessions_ = {};
  // GameManager game_;
};

} // namespace net
} // namespace utils

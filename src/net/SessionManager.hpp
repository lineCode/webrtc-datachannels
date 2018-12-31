#pragma once

#include <memory>
#include <string>
#include <unordered_map>

namespace utils {
namespace net {

class WsSession;

/**
 * @brief manages currently valid sessions
 */
class SessionManager {
public:
  void registerSession(const std::shared_ptr<WsSession>& session);
  void unregisterSession(size_t id);
  void interpret(size_t id, const std::string& message);
  size_t connected() const;
  size_t sessionsCount() const { return sessions_.size(); }

private:
  std::unordered_map<std::size_t, std::weak_ptr<WsSession>> sessions_;
  // GameManager game_;
};

} // namespace net
} // namespace utils

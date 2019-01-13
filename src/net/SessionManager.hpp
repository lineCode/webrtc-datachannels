#pragma once

/*#include <boost/functional/hash.hpp>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.*/

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

/*namespace std {

// specialize std::hash template for boost::uuid.
template <> struct hash<boost::uuids::uuid> {
  size_t operator()(const boost::uuids::uuid& uid) {
    return boost::hash<boost::uuids::uuid>()(uid);
  }
};

} // namespace std*/

namespace utils {
namespace net {

class WsSession;

/**
 * @brief manages currently valid sessions
 */
class SessionManager {
public:
  void registerSession(const std::shared_ptr<WsSession>& session);
  void unregisterSession(std::string id);
  void interpret(size_t id, const std::string& message);
  void sendToAll(const std::string message);
  void handleAllPlayerMessages();
  void doToAllPlayers(std::function<void(std::shared_ptr<WsSession>)> func);
  size_t connected() const;
  size_t sessionsCount() const { return sessions_.size(); }
  uint32_t maxSessionId() const { return maxSessionId_; }
  // TODO
  uint32_t maxSessionId_ = 0;

private:
  std::unordered_map<std::string, std::shared_ptr<WsSession>> sessions_;
  // GameManager game_;
};

} // namespace net
} // namespace utils

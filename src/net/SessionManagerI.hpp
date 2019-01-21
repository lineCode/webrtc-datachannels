#pragma once

#include "log/Logger.hpp"
#include "net/SessionI.hpp"
#include <boost/asio.hpp>
#include <thread>
#include <type_traits>
#include <vector>

namespace utils {
namespace config {
class ServerConfig;
} // namespace config
} // namespace utils

namespace utils {
namespace net {

template <typename sessType, typename callbacksType> class SessionManagerI {
  // static_assert(!std::is_base_of<sessType, SessionI>::value, "sessType must inherit from
  // SessionI");

public:
  SessionManagerI() {}

  virtual ~SessionManagerI() {}

  /**
   * @example:
   * std::time_t t = std::chrono::system_clock::to_time_t(p);
   * std::string msg = "server_time: ";
   * msg += std::ctime(&t);
   * sm->sendToAll(msg);
   **/
  virtual void sendToAll(const std::string& message) = 0;

  virtual void sendTo(const std::string& sessionID, const std::string& message) = 0;

  virtual void handleIncomingMessages() = 0;

  virtual void doToAllSessions(std::function<void(std::shared_ptr<sessType>)> func);

  /**
   * @brief returns the number of connected clients
   *
   * @return number of valid sessions
   */
  virtual size_t getSessionsCount() const { return sessions_.size(); };

  virtual std::unordered_map<std::string, std::shared_ptr<sessType>> getSessions() const {
    return sessions_;
  }

  virtual std::shared_ptr<sessType> getSessById(const std::string& sessionID);

  virtual bool addSession(const std::string& sessionID, std::shared_ptr<sessType> sess);

  virtual void unregisterSession(const std::string& id) = 0;

  virtual callbacksType getOperationCallbacks() const { return operationCallbacks_; }

  virtual void runThreads(const utils::config::ServerConfig& serverConfig) = 0;

  virtual void finishThreads() = 0;

protected:
  callbacksType operationCallbacks_;

  // Used to map SessionId to Session
  std::unordered_map<std::string, std::shared_ptr<sessType>> sessions_ = {};
};

template <typename sessType, typename callbacksType>
std::shared_ptr<sessType>
SessionManagerI<sessType, callbacksType>::getSessById(const std::string& sessionID) {
  {
    auto it = sessions_.find(sessionID);
    if (it != sessions_.end()) {
      return it->second;
    }
  }

  LOG(WARNING) << "WSServer::getSessById: unknown session with id = " << sessionID;
  return nullptr;
}

/**
 * @brief adds a session to list of valid sessions
 *
 * @param session session to be registered
 */
template <typename sessType, typename callbacksType>
bool SessionManagerI<sessType, callbacksType>::addSession(const std::string& sessionID,
                                                          std::shared_ptr<sessType> sess) {
  if (!sess || !sess.get()) {
    LOG(WARNING) << "addSession: Invalid session ";
    return false;
  }
  { sessions_[sessionID] = sess; }
  return true; // TODO: handle collision
}

/**
 * @example:
 * sm->doToAll([&](std::shared_ptr<WsSession> session) {
 *   session.get()->send("Your id: " + session.get()->getId());
 * });
 **/
template <typename sessType, typename callbacksType>
void SessionManagerI<sessType, callbacksType>::doToAllSessions(
    std::function<void(std::shared_ptr<sessType>)> func) {
  {
    for (auto& sessionkv : sessions_) {
      if (auto session = sessionkv.second) {
        if (!session || !session.get()) {
          LOG(WARNING) << "doToAllSessions: Invalid session ";
          continue;
        }
        func(session);
      }
    }
  }
}

} // namespace net
} // namespace utils

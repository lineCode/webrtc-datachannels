#pragma once

#include "log/Logger.hpp"
#include <boost/asio.hpp>
#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

namespace gloer {
namespace config {
struct ServerConfig;
} // namespace config
} // namespace gloer

namespace gloer {
namespace net {

template <typename SessType, typename SessGUID>
class SessionManagerBase {
  // static_assert(!std::is_base_of<SessType, SessionI>::value, "SessType must inherit from
  // SessionI");

public:
  typedef std::function<void(const std::shared_ptr<SessType>& sess)> on_new_sess_callback;

  SessionManagerBase() {}

  virtual ~SessionManagerBase() {}

  virtual void SetOnNewSessionHandler(on_new_sess_callback handler) {
    onNewSessCallback_ = handler;
  }

  virtual void
  doToAllSessions(std::function<void(const SessGUID& sessId, std::shared_ptr<SessType>)> func);

  /**
   * @brief returns the number of connected clients
   *
   * @return number of valid sessions
   */
  virtual size_t getSessionsCount() const {
    std::scoped_lock lock(sessionsMutex_);
    return sessions_.size();
  }

  virtual std::unordered_map<SessGUID, std::shared_ptr<SessType>> getSessions() const {
    std::scoped_lock lock(sessionsMutex_);
    return sessions_;
  }

  virtual std::shared_ptr<SessType> getSessById(const SessGUID& sessionID);

  virtual bool addSession(const SessGUID& sessionID, std::shared_ptr<SessType> sess);

  virtual void unregisterSession(const SessGUID& id) = 0;

  virtual bool removeSessById(const SessGUID& sessionID);

public:
  on_new_sess_callback onNewSessCallback_;

protected:
  // @see stackoverflow.com/a/25521702/10904212
  mutable std::mutex sessionsMutex_;

  // Used to map SessionId to Session
  std::unordered_map<SessGUID, std::shared_ptr<SessType>> sessions_ =
      {}; // TODO: use github.com/facebook/folly/blob/master/folly/docs/Synchronized.md
};

template <typename SessType, typename SessGUID>
bool SessionManagerBase<SessType, SessGUID>::removeSessById(const SessGUID& sessionID) {
  {
    std::scoped_lock lock(sessionsMutex_);
    if (!sessions_.erase(sessionID)) {
      // LOG(WARNING) << "unregisterSession: trying to unregister non-existing session " <<
      // sessionID;

      return false; // NOTE: continue cleanup with saved shared_ptr
    }
  }
  return true;
}

template <typename SessType, typename SessGUID>
std::shared_ptr<SessType>
SessionManagerBase<SessType, SessGUID>::getSessById(const SessGUID& sessionID) {
  {
    std::scoped_lock lock(sessionsMutex_);
    auto it = sessions_.find(sessionID);
    if (it != sessions_.end()) {
      return it->second;
    }
  }

  // LOG(WARNING) << "getSessById: unknown session with id = " << sessionID;
  return nullptr;
}

/**
 * @brief adds a session to list of valid sessions
 *
 * @param session session to be registered
 */
template <typename SessType, typename SessGUID>
bool SessionManagerBase<SessType, SessGUID>::addSession(const SessGUID& sessionID,
                                              std::shared_ptr<SessType> sess) {
  if (!sess || !sess.get()) {
    // LOG(WARNING) << "addSession: Invalid session ";
    return false;
  }
  {
    std::scoped_lock lock(sessionsMutex_);
    sessions_[sessionID] = sess;
  }
  return true; // TODO: handle collision
}

/**
 * @example:
 * sm->doToAll([&](std::shared_ptr<WsSession> session) {
 *   session.get()->send("Your id: " + session.get()->getId());
 * });
 **/
template <typename SessType, typename SessGUID>
void SessionManagerBase<SessType, SessGUID>::doToAllSessions(
    std::function<void(const SessGUID& sessId, std::shared_ptr<SessType>)> func) {
  {
    // NOTE: don`t call getSessions == lock in loop
    const std::unordered_map<SessGUID, std::shared_ptr<SessType>> sessions_copy = getSessions();

    for (auto& sessionkv : sessions_copy) {
      std::shared_ptr<SessType> session = sessionkv.second;
      {
        if (!session || !session.get()) {
          // LOG(WARNING) << "doToAllSessions: Invalid session ";
          continue;
        }
        func(sessionkv.first, session);
      }
    }
  }
}

} // namespace net
} // namespace gloer

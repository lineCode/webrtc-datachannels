#include "net/websockets/WsSessionManager.hpp" // IWYU pragma: associated
#include "algorithm/DispatchQueue.hpp"
#include "log/Logger.hpp"
#include "net/websockets/WsSession.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace utils {
namespace net {

/**
 * @brief adds a session to list of valid sessions
 *
 * @param session session to be registered
 */
void WsSessionManager::registerSession(
    const std::shared_ptr<WsSession>& session) {
  // maxSessionId_ = std::max(session->getId(), maxSessionId_);
  // const std::string wsGuid =
  // boost::lexical_cast<std::string>(session->getId());
  sessions_.insert(std::make_pair(session->getId(), session));
  LOG(INFO) << "total ws sessions: " << getSessionsCount();
}

/**
 * @brief removes session from list of valid sessions
 *
 * @param id id of session to be removed
 */
void WsSessionManager::unregisterSession(std::string id) {
  if (!sessions_.erase(id)) {
    // throw std::runtime_error(
    LOG(WARNING)
        << "WsSessionManager: trying to unregister non-existing session";
  }
  LOG(INFO) << "WsSessionManager: unregistered " << id;
}

/**
 * @brief calls game engine to interpret the message and sends a response to
 * clients
 *
 * @param id id of client who sent the message
 * @param message actual message
 */
void WsSessionManager::interpret(size_t id, const std::string& message) {
  /*auto return_data = game_.doWork(id, message);
  for (auto& msg : return_data) {
    for (auto user_id : msg.second) {
      if (auto session = sessions_.at(user_id).lock()) {
        session->write(msg.first);
      } else {
        //throw std::runtime_error(
            "WsSessionManager: trying to send data to non-existing session");
      }
    }
  }*/
}

/**
 * @example:
 * std::time_t t = std::chrono::system_clock::to_time_t(p);
 * std::string msg = "server_time: ";
 * msg += std::ctime(&t);
 * sm->sendToAll(msg);
 **/
void WsSessionManager::sendToAll(const std::string message) {
  for (auto& sessionkv : sessions_) {
    if (auto session = sessionkv.second.get()) {
      session->send(message);
    }
  }
}

/**
 * @example:
 * sm->doToAll([&](std::shared_ptr<utils::net::WsSession> session) {
 *   session.get()->send("Your id: " + session.get()->getId());
 * });
 **/
void WsSessionManager::doToAllPlayers(
    std::function<void(std::shared_ptr<WsSession>)> func) {
  for (auto& sessionkv : sessions_) {
    if (auto session = sessionkv.second) {
      func(session);
    }
  }
}

void WsSessionManager::handleAllPlayerMessages() {
  doToAllPlayers([&](std::shared_ptr<utils::net::WsSession> session) {
    if (!session) {
      LOG(WARNING) << "WsSessionManager::handleAllPlayerMessages: trying to "
                      "use non-existing session";
      return;
    }
    session->getReceivedMessages()->DispatchQueued();
    // session->getReceivedMessages()->dispatch_loop();
  });
}

} // namespace net
} // namespace utils

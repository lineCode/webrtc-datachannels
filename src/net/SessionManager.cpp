#include "net/SessionManager.hpp"
#include "dispatch_queue.hpp" // for dispatch_queue
#include "net/WsSession.hpp"  // for WsSession
#include <iostream>           // for operator<<, ostream, basic_ostream, cerr
#include <memory>             // for __shared_ptr_access
#include <string>             // for string, basic_string, operator<<, char...
#include <unordered_map>      // for unordered_map
#include <utility>            // for pair, move, make_pair

namespace utils {
namespace net {

/**
 * @brief adds a session to list of valid sessions
 *
 * @param session session to be registered
 */
void SessionManager::registerSession(
    const std::shared_ptr<WsSession>& session) {
  // maxSessionId_ = std::max(session->getId(), maxSessionId_);
  // const std::string wsGuid =
  // boost::lexical_cast<std::string>(session->getId());
  sessions_.insert(std::make_pair(session->getId(), session));
  std::cout << "total ws sessions: " << sessions_.size() << "\n";
}

/**
 * @brief removes session from list of valid sessions
 *
 * @param id id of session to be removed
 */
void SessionManager::unregisterSession(std::string id) {
  if (!sessions_.erase(id)) {
    // throw std::runtime_error(
    std::cerr << "SessionManager: trying to unregister non-existing session\n";
  }
  std::cout << "SessionManager: unregistered " << id << "\n";
}

/**
 * @brief calls game engine to interpret the message and sends a response to
 * clients
 *
 * @param id id of client who sent the message
 * @param message actual message
 */
void SessionManager::interpret(size_t id, const std::string& message) {
  /*auto return_data = game_.doWork(id, message);
  for (auto& msg : return_data) {
    for (auto user_id : msg.second) {
      if (auto session = sessions_.at(user_id).lock()) {
        session->write(msg.first);
      } else {
        //throw std::runtime_error(
            "SessionManager: trying to send data to non-existing session");
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
void SessionManager::sendToAll(const std::string message) {
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
void SessionManager::doToAllPlayers(
    std::function<void(std::shared_ptr<WsSession>)> func) {
  for (auto& sessionkv : sessions_) {
    if (auto session = sessionkv.second) {
      func(session);
    }
  }
}

void SessionManager::handleAllPlayerMessages() {
  doToAllPlayers([&](std::shared_ptr<utils::net::WsSession> session) {
    if (!session) {
      std::cerr << "SessionManager::handleAllPlayerMessages: trying to "
                   "use non-existing session\n";
      return;
    }
    session->getReceivedMessages()->dispatch_queued();
    // session->getReceivedMessages()->dispatch_loop();
  });
}

/**
 * @brief returns the number of connected clients
 *
 * @return number of valid sessions
 */
size_t SessionManager::connected() const { return sessions_.size(); }

} // namespace net
} // namespace utils

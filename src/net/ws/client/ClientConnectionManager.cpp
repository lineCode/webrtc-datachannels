#include "net/ws/client/ClientConnectionManager.hpp" // IWYU pragma: associated
#include "net/ws/client/ClientSessionManager.hpp"
#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManagerBase.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/wrtc/WRTCSession.hpp"
#include "net/wrtc/wrtc.hpp"
#include "net/ws/WsNetworkOperation.hpp"
#include "net/ws/server/ServerSession.hpp"
#include "net/ws/client/ClientSession.hpp"
#include "net/SessionBase.hpp"
#include "net/SessionPair.hpp"
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <cstddef>
#include <iostream>
#include <memory>
#include <net/core.hpp>
#include <rapidjson/document.h>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <webrtc/rtc_base/bind.h>
#include <webrtc/rtc_base/checks.h>
#include "net/ws/client/WSClientNetworkManager.hpp"
#include "net/ws/server/WSServerNetworkManager.hpp"

namespace gloer {
namespace net {
namespace ws {

ClientConnectionManager::ClientConnectionManager(net::WSClientNetworkManager* nm, const gloer::config::ServerConfig& serverConfig, ws::ClientSessionManager& sm)
    : nm_(nm), ioc_(serverConfig.threads_), sm_(sm)
    // The SSL context is required, and holds certificates
    , ctx_{::boost::asio::ssl::context::tlsv12} {
  /*  const ws::WsNetworkOperation PING_OPERATION =
        ws::WsNetworkOperation(algo::WS_OPCODE::PING,
    algo::Opcodes::opcodeToStr(algo::WS_OPCODE::PING)); addCallback(PING_OPERATION, &pingCallback);

    const ws::WsNetworkOperation CANDIDATE_OPERATION = ws::WsNetworkOperation(
        algo::WS_OPCODE::CANDIDATE, algo::Opcodes::opcodeToStr(algo::WS_OPCODE::CANDIDATE));
    addCallback(CANDIDATE_OPERATION, &candidateCallback);

    const ws::WsNetworkOperation OFFER_OPERATION = ws::WsNetworkOperation(
        algo::WS_OPCODE::OFFER, algo::Opcodes::opcodeToStr(algo::WS_OPCODE::OFFER));
    addCallback(OFFER_OPERATION, &offerCallback);

    const ws::WsNetworkOperation ANSWER_OPERATION = ws::WsNetworkOperation(
        algo::WS_OPCODE::ANSWER, algo::Opcodes::opcodeToStr(algo::WS_OPCODE::ANSWER));
    addCallback(ANSWER_OPERATION, &answerCallback);*/
}

void ClientConnectionManager::addCallback(const ws::WsNetworkOperation& op, const ClientNetworkOperationCallback& cb) {
  nm_->operationCallbacks().addCallback(op, cb);
}

#if 0
/**
 * @brief removes session from list of valid sessions
 *
 * @param id id of session to be removed
 */
void ClientConnectionManager::unregisterSession(const ws::SessionGUID& id) {
  LOG(WARNING) << "unregisterSession for id = " << static_cast<std::string>(id);
  const ws::SessionGUID idCopy = id; // unknown lifetime, use idCopy
  std::shared_ptr<SessionPair> sess = getSessById(idCopy);

  {
    if (!removeSessById(idCopy)) {
      // LOG(WARNING) << "ClientConnectionManager::unregisterSession: trying to unregister non-existing session "
      //             << idCopy;
      // NOTE: continue cleanup with saved shared_ptr
    }
    if (!sess) {
      // throw std::runtime_error(
      // LOG(WARNING) << "ClientConnectionManager::unregisterSession: session already deleted";
      return;
    }
  }

  // close conn, e.t.c.
  /*if (sess && sess.get()) {
    sess->close();
  }*/

  // LOG(WARNING) << "Client: unregistered " << idCopy;
}
#endif // 0

/**
 * @example:
 * std::time_t t = std::chrono::system_clock::to_time_t(p);
 * std::string msg = "server_time: ";
 * msg += std::ctime(&t);
 * sm->sendToAll(msg);
 **/
void ClientConnectionManager::sendToAll(const std::string& message) {
  LOG(WARNING) << "ClientConnectionManager::sendToAll:" << message;
  {
    // NOTE: don`t call getSessions == lock in loop
    const auto sessionsCopy = sm_.getSessions();

    for (auto& sessionkv : sessionsCopy) {
      if (!sessionkv.second || !sessionkv.second.get()) {
        LOG(WARNING) << "ClientConnectionManager::sendToAll: Invalid session ";
        continue;
      }
      if (auto session = sessionkv.second.get()) {
        session->send(message);
      }
    }
  }
}

void ClientConnectionManager::sendTo(const ws::SessionGUID& sessionID, const std::string& message) {
  {
    // NOTE: don`t call getSessions == lock in loop
    const auto sessionsCopy = sm_.getSessions();

    auto it = sessionsCopy.find(sessionID);
    if (it != sessionsCopy.end()) {
      if (!it->second || !it->second.get()) {
        LOG(WARNING) << "ClientConnectionManager::sendTo: Invalid session ";
        return;
      }
      it->second->send(message);
    }
  }
}


std::shared_ptr<ClientSession> ClientConnectionManager::addClientSession(
  const ws::SessionGUID& newSessId)
{
  auto newWsSession = std::make_shared<ClientSession>(
    ioc_,
    ctx_,
    nm_,
    newSessId);

  sm_.addSession(newSessId, newWsSession);
  return newWsSession;
}


void ClientConnectionManager::run(const config::ServerConfig& serverConfig) {
  wsThreads_.reserve(serverConfig.threads_);
  for (auto i = serverConfig.threads_; i > 0; --i) {
    wsThreads_.emplace_back([this] { ioc_.run(); });
  }
  // TODO sigWait(ioc);
  // TODO ioc.run();
}

void ClientConnectionManager::finish() {
  // Block until all the threads exit
  for (auto& t : wsThreads_) {
    if (t.joinable()) {
      t.join();
    }
  }
}

void ClientConnectionManager::prepare(const gloer::config::ServerConfig &serverConfig)
{}

} // namespace ws
} // namespace net
} // namespace gloer

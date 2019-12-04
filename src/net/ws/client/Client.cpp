#include "net/ws/client/Client.hpp" // IWYU pragma: associated
#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManagerBase.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/wrtc/WRTCSession.hpp"
#include "net/wrtc/wrtc.hpp"
#include "net/ws/WsListener.hpp"
#include "net/ws/WsSession.hpp"
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

namespace gloer {
namespace net {
namespace ws {

Client::Client(net::WSClientNetworkManager* nm, const gloer::config::ServerConfig& serverConfig, ws::ClientSessionManager& sm)
    : nm_(nm), ioc_(serverConfig.threads_), sm_(sm)
    // The SSL context is required, and holds certificates
    , ctx_{::boost::asio::ssl::context::tlsv12} {
  /*  const WsNetworkOperation PING_OPERATION =
        WsNetworkOperation(algo::WS_OPCODE::PING,
    algo::Opcodes::opcodeToStr(algo::WS_OPCODE::PING)); addCallback(PING_OPERATION, &pingCallback);

    const WsNetworkOperation CANDIDATE_OPERATION = WsNetworkOperation(
        algo::WS_OPCODE::CANDIDATE, algo::Opcodes::opcodeToStr(algo::WS_OPCODE::CANDIDATE));
    addCallback(CANDIDATE_OPERATION, &candidateCallback);

    const WsNetworkOperation OFFER_OPERATION = WsNetworkOperation(
        algo::WS_OPCODE::OFFER, algo::Opcodes::opcodeToStr(algo::WS_OPCODE::OFFER));
    addCallback(OFFER_OPERATION, &offerCallback);

    const WsNetworkOperation ANSWER_OPERATION = WsNetworkOperation(
        algo::WS_OPCODE::ANSWER, algo::Opcodes::opcodeToStr(algo::WS_OPCODE::ANSWER));
    addCallback(ANSWER_OPERATION, &answerCallback);*/
}

void Client::addCallback(const WsNetworkOperation& op, const WsClientNetworkOperationCallback& cb) {
  nm_->operationCallbacks().addCallback(op, cb);
}

#if 0
/**
 * @brief removes session from list of valid sessions
 *
 * @param id id of session to be removed
 */
void Client::unregisterSession(const std::string& id) {
  LOG(WARNING) << "unregisterSession for id = " << id;
  const std::string idCopy = id; // unknown lifetime, use idCopy
  std::shared_ptr<SessionPair> sess = getSessById(idCopy);

  // close conn, e.t.c.
  if (sess && sess.get()) {
    sess->close();
  }

  {
    if (!removeSessById(idCopy)) {
      // LOG(WARNING) << "Client::unregisterSession: trying to unregister non-existing session "
      //             << idCopy;
      // NOTE: continue cleanup with saved shared_ptr
    }
    if (!sess) {
      // throw std::runtime_error(
      // LOG(WARNING) << "Client::unregisterSession: session already deleted";
      return;
    }
  }
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
void Client::sendToAll(const std::string& message) {
  LOG(WARNING) << "Client::sendToAll:" << message;
  {
    // NOTE: don`t call getSessions == lock in loop
    const auto sessionsCopy = sm_.getSessions();

    for (auto& sessionkv : sessionsCopy) {
      if (!sessionkv.second || !sessionkv.second.get()) {
        LOG(WARNING) << "Client::sendToAll: Invalid session ";
        continue;
      }
      if (auto session = sessionkv.second.get()) {
        session->send(message);
      }
    }
  }
}

void Client::sendTo(const std::string& sessionID, const std::string& message) {
  {
    // NOTE: don`t call getSessions == lock in loop
    const auto sessionsCopy = sm_.getSessions();

    auto it = sessionsCopy.find(sessionID);
    if (it != sessionsCopy.end()) {
      if (!it->second || !it->second.get()) {
        LOG(WARNING) << "Client::sendTo: Invalid session ";
        return;
      }
      it->second->send(message);
    }
  }
}


std::shared_ptr<ClientSession> Client::addClientSession(
  const std::string& newSessId)
{
  auto newWsSession = std::make_shared<ClientSession>(
    ioc_,
    ctx_,
    nm_,
    newSessId);

  sm_.addSession(newSessId, newWsSession);
  return newWsSession;
}


void Client::runThreads_t(const config::ServerConfig& serverConfig) {
  wsThreads_.reserve(serverConfig.threads_);
  for (auto i = serverConfig.threads_; i > 0; --i) {
    wsThreads_.emplace_back([this] { ioc_.run(); });
  }
  // TODO sigWait(ioc);
  // TODO ioc.run();
}

void Client::finishThreads_t() {
  // Block until all the threads exit
  for (auto& t : wsThreads_) {
    if (t.joinable()) {
      t.join();
    }
  }
}

void Client::prepare(const gloer::config::ServerConfig &serverConfig)
{}

} // namespace ws
} // namespace net
} // namespace gloer

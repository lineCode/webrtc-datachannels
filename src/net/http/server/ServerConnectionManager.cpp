#include "net/http/server/ServerConnectionManager.hpp" // IWYU pragma: associated
#include <net/http/SessionGUID.hpp>
#include "net/http/server/ServerSession.hpp"
#include "net/http/server/ServerSessionManager.hpp"
#include "algo/DispatchQueue.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/http/server/HTTPServerNetworkManager.hpp"
#include "net/http/server/ServerSessionManager.hpp"
#include "net/http/server/ServerInputCallbacks.hpp"
#include "net/ws/client/WSClientNetworkManager.hpp"
#include "net/ws/server/WSServerNetworkManager.hpp"
/*#include "net/wrtc/WRTCServer.hpp"
#include "net/wrtc/WRTCSession.hpp"
#include "net/wrtc/wrtc.hpp"*/
#include "net/ws/server/Listener.hpp" /// TODO: move from ws to http
#include "net/SessionBase.hpp"
#include "net/http/HTTPNetworkOperation.hpp"
#include "net/NetworkManagerBase.hpp"
#include "algo/NetworkOperation.hpp"
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
namespace http {

// TODO: add webrtc callbacks (similar to websockets)

ServerConnectionManager::ServerConnectionManager(
  net::http::HTTPServerNetworkManager* http_nm,
  net::WSServerNetworkManager* ws_nm,
  const gloer::config::ServerConfig& serverConfig,
  http::ServerSessionManager& sm)
    : ws_nm_(ws_nm), http_nm_(http_nm), ioc_(serverConfig.threads_), sm_(sm)
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

void ServerConnectionManager::addCallback(const http::HTTPNetworkOperation& op, const ServerNetworkOperationCallback& cb) {
  http_nm_->operationCallbacks().addCallback(op, cb);
}

/**
 * @example:
 * std::time_t t = std::chrono::system_clock::to_time_t(p);
 * std::string msg = "server_time: ";
 * msg += std::ctime(&t);
 * sm->sendToAll(msg);
 **/
void ServerConnectionManager::sendToAll(const std::string& message) {
  LOG(WARNING) << "ServerConnectionManager::sendToAll:" << message;
  {
    // NOTE: don`t call getSessions == lock in loop
    const auto sessionsCopy = sm_.getSessions();

    for (auto& sessionkv : sessionsCopy) {
      if (!sessionkv.second || !sessionkv.second.get()) {
        LOG(WARNING) << "ServerConnectionManager::sendToAll: Invalid session ";
        continue;
      }
      if (auto session = sessionkv.second.get()) {
        session->send(message);
      }
    }
  }
}

void ServerConnectionManager::sendTo(const http::SessionGUID& sessionID, const std::string& message) {
  {
    // NOTE: don`t call getSessions == lock in loop
    const auto sessionsCopy = sm_.getSessions();

    auto it = sessionsCopy.find(sessionID);
    if (it != sessionsCopy.end()) {
      if (!it->second || !it->second.get()) {
        LOG(WARNING) << "ServerConnectionManager::sendTo: Invalid session ";
        return;
      }
      it->second->send(message);
    }
  }
}

void ServerConnectionManager::run(const config::ServerConfig& serverConfig) {
  httpThreads_.reserve(serverConfig.threads_);
  for (auto i = serverConfig.threads_; i > 0; --i) {
    httpThreads_.emplace_back([this] { ioc_.run(); });
  }
  // TODO sigWait(ioc);
  // TODO ioc.run();
}

void ServerConnectionManager::finish() {
  // Block until all the threads exit
  for (auto& t : httpThreads_) {
    if (t.joinable()) {
      t.join();
    }
  }
}

void ServerConnectionManager::prepare(const config::ServerConfig& serverConfig) {
  initListener(serverConfig);
  RTC_DCHECK(HTTPAndWSListener_);
  HTTPAndWSListener_->run(/*WS_LISTEN_MODE::BOTH*/);
}

std::shared_ptr<ws::Listener> ServerConnectionManager::getListener() const { return HTTPAndWSListener_; }

void ServerConnectionManager::initListener(const config::ServerConfig& serverConfig) {

  const ::tcp::endpoint tcpEndpoint = ::tcp::endpoint{serverConfig.address_, serverConfig.wsPort_};

  std::shared_ptr<std::string const> workdirPtr =
      std::make_shared<std::string>(serverConfig.workdir_.string());

  if (!workdirPtr || !workdirPtr.get()) {
    LOG(WARNING) << "ServerConnectionManager::runIocWsListener: Invalid workdirPtr";
    return;
  }

  // Create and launch a listening port
  HTTPAndWSListener_ = std::make_shared<ws::Listener>(ioc_, ctx_,
    tcpEndpoint, workdirPtr, http_nm_, ws_nm_);
  if (!HTTPAndWSListener_ || !HTTPAndWSListener_.get()) {
    LOG(WARNING) << "ServerConnectionManager::runIocWsListener: Invalid iocWsListener_";
    return;
  }
}

} // namespace http
} // namespace net
} // namespace gloer

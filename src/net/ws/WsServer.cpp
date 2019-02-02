#include "net/ws/WsServer.hpp" // IWYU pragma: associated
#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/wrtc/WRTCSession.hpp"
#include "net/ws/WsListener.hpp"
#include "net/ws/WsSession.hpp"
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
#include <webrtc/rtc_base/rtccertificategenerator.h>
#include <webrtc/rtc_base/ssladapter.h>

namespace {

using namespace ::gloer::net;
using namespace ::gloer::net::wrtc;
using namespace ::gloer::net::ws;

static void pingCallback(std::shared_ptr<WsSession> clientSession, NetworkManager* nm,
                         std::shared_ptr<std::string> messageBuffer) {
  if (!messageBuffer || !messageBuffer.get() || messageBuffer->empty()) {
    LOG(WARNING) << "WsServer: Invalid messageBuffer";
    return;
  }

  if (!clientSession || !clientSession.get()) {
    LOG(WARNING) << "WSServer invalid clientSession!";
    return;
  }

  if (!nm) {
    LOG(WARNING) << "WSServer invalid NetworkManager!";
    return;
  }

  std::string dataCopy = *messageBuffer.get();

  // const std::string incomingStr = beast::buffers_to_string(messageBuffer->data());
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "pingCallback incomingMsg=" << messageBuffer->c_str();

  // send same message back (ping-pong)
  if (clientSession && clientSession.get() && clientSession->isOpen() &&
      !clientSession->isExpired())
    clientSession->send(dataCopy);
}

static void candidateCallback(std::shared_ptr<WsSession> clientSession, NetworkManager* nm,
                              std::shared_ptr<std::string> messageBuffer) {
  // const std::string incomingStr = beast::buffers_to_string(messageBuffer->data());

  if (!messageBuffer || !messageBuffer.get() || messageBuffer->empty()) {
    LOG(WARNING) << "WsServer: Invalid messageBuffer";
    return;
  }

  if (!clientSession || !clientSession.get()) {
    LOG(WARNING) << "WSServer invalid clientSession!";
    return;
  }

  if (!nm) {
    LOG(WARNING) << "WSServer invalid NetworkManager!";
    return;
  }

  std::string dataCopy = *messageBuffer.get();

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "candidateCallback incomingMsg=" << dataCopy;

  // todo: pass parsed
  rapidjson::Document message_object;
  message_object.Parse(dataCopy.c_str());

  // Server receives Client’s ICE candidates, then finds its own ICE
  // candidates & sends them to Client
  LOG(INFO) << "type == candidate";

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "m_WS->WSQueue.dispatch type == candidate";
  rapidjson::Document message_object1;     // TODO
  message_object1.Parse(dataCopy.c_str()); // TODO

  auto spt =
      clientSession->getWRTCSession().lock(); // Has to be copied into a shared_ptr before usage

  /*auto handle = OnceFunctor([clientSession, spt, nm, &message_object1]() {
    if (spt) {
      spt->createAndAddIceCandidate(message_object1);
    } else {
      LOG(WARNING) << "wrtcSess_ expired";
      return;
    }
  });

  nm->getWRTC()->workerThread_->Post(RTC_FROM_HERE, handle);*/

  if (spt) {
    spt->createAndAddIceCandidate(message_object1);
  } else {
    LOG(WARNING) << "wrtcSess_ expired";
    return;
  }
}

static void offerCallback(std::shared_ptr<WsSession> clientSession, NetworkManager* nm,
                          std::shared_ptr<std::string> messageBuffer) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WS: type == offer";

  if (!messageBuffer || !messageBuffer.get() || messageBuffer->empty()) {
    LOG(WARNING) << "WsServer: Invalid messageBuffer";
    return;
  }

  if (!clientSession || !clientSession.get()) {
    LOG(WARNING) << "WSServer invalid clientSession!";
    return;
  }

  if (!nm) {
    LOG(WARNING) << "WSServer invalid NetworkManager!";
    return;
  }

  std::string dataCopy = *messageBuffer.get();

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "offerCallback incomingMsg=" << dataCopy.c_str();

  // todo: pass parsed
  rapidjson::Document message_object;
  message_object.Parse(dataCopy.c_str());

  // TODO: don`t create datachennel for same client twice?
  LOG(INFO) << "type == offer";

  rapidjson::Document message_obj;     // TODO
  message_obj.Parse(dataCopy.c_str()); // TODO

  const auto sdp = WRTCServer::sessionDescriptionStrFromJson(message_obj);

  /*auto handle = OnceFunctor([clientSession, nm, sdp]() {
    WRTCServer::setRemoteDescriptionAndCreateAnswer(clientSession, nm, sdp);
  });
  nm->getWRTC()->workerThread_->Post(RTC_FROM_HERE, handle);*/
  WRTCServer::setRemoteDescriptionAndCreateAnswer(clientSession, nm, sdp);

  LOG(INFO) << "WS: added type == offer";
}

// TODO: answerCallback unused
static void answerCallback(std::shared_ptr<WsSession> clientSession, NetworkManager* nm,
                           std::shared_ptr<std::string> messageBuffer) {
  if (!messageBuffer || !messageBuffer.get() || messageBuffer->empty()) {
    LOG(WARNING) << "WsServer: Invalid messageBuffer";
    return;
  }

  if (!clientSession || !clientSession.get()) {
    LOG(WARNING) << "WSServer invalid clientSession!";
    return;
  }

  if (!nm) {
    LOG(WARNING) << "WSServer invalid NetworkManager!";
    return;
  }

  std::string dataCopy = *messageBuffer.get();

  // const std::string incomingStr = beast::buffers_to_string(messageBuffer->data());
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "answerCallback incomingMsg=" << dataCopy;
  // send same message back (ping-pong)
  // clientSession->send(incomingStr);
}

} // namespace

namespace gloer {
namespace net {
namespace ws {

WSInputCallbacks::WSInputCallbacks() {}

WSInputCallbacks::~WSInputCallbacks() {}

std::map<WsNetworkOperation, WsNetworkOperationCallback> WSInputCallbacks::getCallbacks() const {
  return operationCallbacks_;
}

void WSInputCallbacks::addCallback(const WsNetworkOperation& op,
                                   const WsNetworkOperationCallback& cb) {
  operationCallbacks_[op] = cb;
}

// TODO: add webrtc callbacks (similar to websockets)

WSServer::WSServer(NetworkManager* nm, const gloer::config::ServerConfig& serverConfig)
    : nm_(nm), ioc_(serverConfig.threads_) {
  const WsNetworkOperation PING_OPERATION =
      WsNetworkOperation(algo::WS_OPCODE::PING, algo::Opcodes::opcodeToStr(algo::WS_OPCODE::PING));
  operationCallbacks_.addCallback(PING_OPERATION, &pingCallback);

  const WsNetworkOperation CANDIDATE_OPERATION = WsNetworkOperation(
      algo::WS_OPCODE::CANDIDATE, algo::Opcodes::opcodeToStr(algo::WS_OPCODE::CANDIDATE));
  operationCallbacks_.addCallback(CANDIDATE_OPERATION, &candidateCallback);

  const WsNetworkOperation OFFER_OPERATION = WsNetworkOperation(
      algo::WS_OPCODE::OFFER, algo::Opcodes::opcodeToStr(algo::WS_OPCODE::OFFER));
  operationCallbacks_.addCallback(OFFER_OPERATION, &offerCallback);

  const WsNetworkOperation ANSWER_OPERATION = WsNetworkOperation(
      algo::WS_OPCODE::ANSWER, algo::Opcodes::opcodeToStr(algo::WS_OPCODE::ANSWER));
  operationCallbacks_.addCallback(ANSWER_OPERATION, &answerCallback);
}

/**
 * @brief removes session from list of valid sessions
 *
 * @param id id of session to be removed
 */
void WSServer::unregisterSession(const std::string& id) {
  const std::string idCopy = id; // unknown lifetime, use idCopy
  std::shared_ptr<WsSession> sess = getSessById(idCopy);

  // close conn, e.t.c.
  if (sess && sess.get()) {
    sess->close();
  }

  {
    if (!removeSessById(idCopy)) {
      // LOG(WARNING) << "WsServer::unregisterSession: trying to unregister non-existing session "
      //             << idCopy;
      // NOTE: continue cleanup with saved shared_ptr
    }
    if (!sess) {
      // throw std::runtime_error(
      // LOG(WARNING) << "WsServer::unregisterSession: session already deleted";
      return;
    }
  }
  // LOG(WARNING) << "WsServer: unregistered " << idCopy;
}

/**
 * @example:
 * std::time_t t = std::chrono::system_clock::to_time_t(p);
 * std::string msg = "server_time: ";
 * msg += std::ctime(&t);
 * sm->sendToAll(msg);
 **/
void WSServer::sendToAll(const std::string& message) {
  LOG(WARNING) << "WSServer::sendToAll:" << message;
  {
    // NOTE: don`t call getSessions == lock in loop
    const auto sessionsCopy = getSessions();

    for (auto& sessionkv : sessionsCopy) {
      if (!sessionkv.second || !sessionkv.second.get()) {
        LOG(WARNING) << "WSServer::sendToAll: Invalid session ";
        continue;
      }
      if (auto session = sessionkv.second.get()) {
        session->send(message);
      }
    }
  }
}

void WSServer::sendTo(const std::string& sessionID, const std::string& message) {
  {
    // NOTE: don`t call getSessions == lock in loop
    const auto sessionsCopy = getSessions();

    auto it = sessionsCopy.find(sessionID);
    if (it != sessionsCopy.end()) {
      if (!it->second || !it->second.get()) {
        LOG(WARNING) << "WSServer::sendTo: Invalid session ";
        return;
      }
      it->second->send(message);
    }
  }
}

void WSServer::runThreads_t(const config::ServerConfig& serverConfig) {
  wsThreads_.reserve(serverConfig.threads_);
  for (auto i = serverConfig.threads_; i > 0; --i) {
    wsThreads_.emplace_back([this] { ioc_.run(); });
  }
  // TODO sigWait(ioc);
  // TODO ioc.run();
}

void WSServer::finishThreads_t() {
  // Block until all the threads exit
  for (auto& t : wsThreads_) {
    if (t.joinable()) {
      t.join();
    }
  }
}

void WSServer::runAsServer(const config::ServerConfig& serverConfig) {
  initListener(serverConfig);
  wsListener_->run(WS_LISTEN_MODE::BOTH);
}

void WSServer::runAsClient(const config::ServerConfig& serverConfig) {
  initListener(serverConfig);
  wsListener_->run(WS_LISTEN_MODE::CLIENT);
}

std::shared_ptr<WsListener> WSServer::getListener() const { return wsListener_; }

void WSServer::initListener(const config::ServerConfig& serverConfig) {

  const ::tcp::endpoint tcpEndpoint = ::tcp::endpoint{serverConfig.address_, serverConfig.wsPort_};

  std::shared_ptr<std::string const> workdirPtr =
      std::make_shared<std::string>(serverConfig.workdir_.string());

  if (!workdirPtr || !workdirPtr.get()) {
    LOG(WARNING) << "WSServer::runIocWsListener: Invalid workdirPtr";
    return;
  }

  // Create and launch a listening port
  wsListener_ = std::make_shared<WsListener>(ioc_, tcpEndpoint, workdirPtr, nm_);
  if (!wsListener_ || !wsListener_.get()) {
    LOG(WARNING) << "WSServer::runIocWsListener: Invalid iocWsListener_";
    return;
  }
}

} // namespace ws
} // namespace net
} // namespace gloer

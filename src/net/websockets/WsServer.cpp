#include "net/websockets/WsServer.hpp" // IWYU pragma: associated
#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/webrtc/WRTCServer.hpp"
#include "net/webrtc/WRTCSession.hpp"
#include "net/websockets/WsListener.hpp"
#include "net/websockets/WsSession.hpp"
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <cstddef>
#include <iostream>
#include <memory>
#include <rapidjson/document.h>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace {

using namespace ::gloer::net;

static void pingCallback(WsSession* clientSession, NetworkManager* nm,
                         std::shared_ptr<std::string> messageBuffer) {
  if (!messageBuffer || !messageBuffer.get()) {
    LOG(WARNING) << "WsServer: Invalid messageBuffer";
    return;
  }

  if (!clientSession) {
    LOG(WARNING) << "WSServer invalid clientSession!";
    return;
  }

  // const std::string incomingStr = beast::buffers_to_string(messageBuffer->data());
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "pingCallback incomingMsg=" << messageBuffer->c_str();

  // send same message back (ping-pong)
  clientSession->send(messageBuffer);
}

static void candidateCallback(WsSession* clientSession, NetworkManager* nm,
                              std::shared_ptr<std::string> messageBuffer) {
  // const std::string incomingStr = beast::buffers_to_string(messageBuffer->data());

  if (!messageBuffer || !messageBuffer.get()) {
    LOG(WARNING) << "WsServer: Invalid messageBuffer";
    return;
  }

  if (!clientSession) {
    LOG(WARNING) << "WSServer invalid clientSession!";
    return;
  }

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "candidateCallback incomingMsg=" << messageBuffer->c_str();

  // todo: pass parsed
  rapidjson::Document message_object;
  message_object.Parse(messageBuffer->c_str());

  // Server receives Client’s ICE candidates, then finds its own ICE
  // candidates & sends them to Client
  LOG(INFO) << "type == candidate";

  /*if (!clientSession->getWRTCQueue()) {
    LOG(WARNING) << "WSServer::OnWebSocketMessage invalid m_WRTC->WRTCQueue!";
    return;
  }*/

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "m_WRTC->WRTCQueue.dispatch type == candidate";
  rapidjson::Document message_object1;           // TODO
  message_object1.Parse(messageBuffer->c_str()); // TODO

  auto spt =
      clientSession->getWRTCSession().lock(); // Has to be copied into a shared_ptr before usage
  if (spt) {
    spt->createAndAddIceCandidate(message_object1);
  } else {
    LOG(WARNING) << "wrtcSess_ expired";
    return;
  }
}

static void offerCallback(WsSession* clientSession, NetworkManager* nm,
                          std::shared_ptr<std::string> messageBuffer) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WS: type == offer";

  if (!messageBuffer || !messageBuffer.get()) {
    LOG(WARNING) << "WsServer: Invalid messageBuffer";
    return;
  }

  if (!clientSession) {
    LOG(WARNING) << "WSServer invalid clientSession!";
    return;
  }

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "offerCallback incomingMsg=" << messageBuffer->c_str();

  // todo: pass parsed
  rapidjson::Document message_object;
  message_object.Parse(messageBuffer->c_str());

  // TODO: don`t create datachennel for same client twice?
  LOG(INFO) << "type == offer";

  /*if (!clientSession->getWRTCQueue()) {
    LOG(WARNING) << "WSServer::OnWebSocketMessage invalid m_WRTC->WRTCQueue!";
    return;
  }*/

  rapidjson::Document message_obj;           // TODO
  message_obj.Parse(messageBuffer->c_str()); // TODO
  WRTCServer::setRemoteDescriptionAndCreateAnswer(clientSession, nm, message_obj);

  LOG(INFO) << "WS: added type == offer";
}

// TODO: answerCallback unused
static void answerCallback(WsSession* clientSession, NetworkManager* nm,
                           std::shared_ptr<std::string> messageBuffer) {
  if (!messageBuffer || !messageBuffer.get()) {
    LOG(WARNING) << "WsServer: Invalid messageBuffer";
    return;
  }

  if (!clientSession) {
    LOG(WARNING) << "WSServer invalid clientSession!";
    return;
  }

  // const std::string incomingStr = beast::buffers_to_string(messageBuffer->data());
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "answerCallback incomingMsg=" << messageBuffer->c_str();
  // send same message back (ping-pong)
  // clientSession->send(incomingStr);
}

} // namespace

namespace gloer {
namespace net {

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
  {
    std::shared_ptr<WsSession> sess = getSessById(idCopy);
    if (!removeSessById(idCopy)) {
      LOG(WARNING) << "WsServer::unregisterSession: trying to unregister non-existing session";
      // NOTE: continue cleanup with saved shared_ptr
    }
    if (!sess) {
      // throw std::runtime_error(
      LOG(WARNING) << "WsServer::unregisterSession: session already deleted";
      return;
    }
  }
  LOG(WARNING) << "WsServer: unregistered " << idCopy;
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
    for (auto& sessionkv : getSessions()) {
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
    auto sessionsCopy = getSessions();
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

void WSServer::handleIncomingMessages() {
  LOG(INFO) << "WSServer::handleIncomingMessages getSessionsCount " << getSessionsCount();
  doToAllSessions([&](const std::string& sessId, std::shared_ptr<WsSession> session) {
    if (!session || !session.get()) {
      LOG(WARNING) << "WsServer::handleAllPlayerMessages: trying to "
                      "use non-existing session";
      // NOTE: unregisterSession must be automatic!
      unregisterSession(sessId);
      return;
    }
    /*if (!session->isOpen() && session->fullyCreated()) {
      LOG(WARNING) << "WsServer::handleAllPlayerMessages: !session->isOpen()";
      // NOTE: unregisterSession must be automatic!
      unregisterSession(session->getId());
      return;
    }*/
    // LOG(INFO) << "doToAllSessions for " << session->getId();
    auto msgs = session->getReceivedMessages();
    if (!msgs || !msgs.get()) {
      LOG(WARNING) << "WsServer::handleAllPlayerMessages: invalid session->getReceivedMessages()";
      return;
    }
    msgs->DispatchQueued();
  });
}

void WSServer::runThreads(const config::ServerConfig& serverConfig) {
  wsThreads_.reserve(serverConfig.threads_);
  for (auto i = serverConfig.threads_; i > 0; --i) {
    wsThreads_.emplace_back([this] { ioc_.run(); });
  }
  // TODO sigWait(ioc);
  // TODO ioc.run();
}

void WSServer::finishThreads() {
  // Block until all the threads exit
  for (auto& t : wsThreads_) {
    if (t.joinable()) {
      t.join();
    }
  }
}

void WSServer::runIocWsListener(const config::ServerConfig& serverConfig) {

  const tcp::endpoint tcpEndpoint = tcp::endpoint{serverConfig.address_, serverConfig.wsPort_};

  std::shared_ptr<std::string const> workdirPtr =
      std::make_shared<std::string>(serverConfig.workdir_.string());

  if (!workdirPtr || !workdirPtr.get()) {
    LOG(WARNING) << "WSServer::runIocWsListener: Invalid workdirPtr";
    return;
  }

  // Create and launch a listening port
  iocWsListener_ = std::make_shared<WsListener>(ioc_, tcpEndpoint, workdirPtr, nm_);
  if (!iocWsListener_ || !iocWsListener_.get()) {
    LOG(WARNING) << "WSServer::runIocWsListener: Invalid iocWsListener_";
    return;
  }

  iocWsListener_->run();
}

} // namespace net
} // namespace gloer

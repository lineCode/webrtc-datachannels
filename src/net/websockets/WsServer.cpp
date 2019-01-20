#include "net/websockets/WsServer.hpp" // IWYU pragma: associated
#include "algorithm/DispatchQueue.hpp"
#include "algorithm/NetworkOperation.hpp"
#include "log/Logger.hpp"
#include "net/webrtc/WRTCServer.hpp"
#include "net/webrtc/WRTCSession.hpp"
#include "net/websockets/WsSession.hpp"
#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <memory>
#include <rapidjson/document.h>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace utils {
namespace net {

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

void pingCallback(WsSession* clientSession, NetworkManager* nm,
                  std::shared_ptr<beast::multi_buffer> messageBuffer) {
  if (!messageBuffer || !messageBuffer.get()) {
    LOG(WARNING) << "WsServer: Invalid messageBuffer";
    return;
  }

  if (!clientSession) {
    LOG(WARNING) << "WSServer invalid clientSession!";
    return;
  }

  const std::string incomingStr = beast::buffers_to_string(messageBuffer->data());
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "pingCallback incomingMsg=" << incomingStr;
  // send same message back (ping-pong)
  clientSession->send(incomingStr);
}

void candidateCallback(WsSession* clientSession, NetworkManager* nm,
                       std::shared_ptr<beast::multi_buffer> messageBuffer) {
  const std::string incomingStr = beast::buffers_to_string(messageBuffer->data());
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "candidateCallback incomingMsg=" << incomingStr;

  if (!messageBuffer || !messageBuffer.get()) {
    LOG(WARNING) << "WsServer: Invalid messageBuffer";
    return;
  }

  if (!clientSession) {
    LOG(WARNING) << "WSServer invalid clientSession!";
    return;
  }

  // todo: pass parsed
  rapidjson::Document message_object;
  auto msgPayload = beast::buffers_to_string(messageBuffer->data());
  message_object.Parse(msgPayload.c_str());
  LOG(INFO) << msgPayload.c_str();

  // Server receives Client’s ICE candidates, then finds its own ICE
  // candidates & sends them to Client
  LOG(INFO) << "type == candidate";

  if (!clientSession->getWRTC()) {
    LOG(WARNING) << "WSServer::OnWebSocketMessage invalid m_WRTC!";
    return;
  }

  if (!clientSession->getWRTCQueue()) {
    LOG(WARNING) << "WSServer::OnWebSocketMessage invalid m_WRTC->WRTCQueue!";
    return;
  }

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "m_WRTC->WRTCQueue.dispatch type == candidate";
  rapidjson::Document message_object1;       // TODO
  message_object1.Parse(msgPayload.c_str()); // TODO

  auto spt =
      clientSession->getWRTCSession().lock(); // Has to be copied into a shared_ptr before usage
  if (spt) {
    spt->createAndAddIceCandidate(message_object1);
  } else {
    LOG(WARNING) << "wrtcSess_ expired";
    return;
  }

  /*m_WRTC->WRTCQueue->dispatch([m_WRTC, msgPayload] {
          LOG(INFO) << std::this_thread::get_id() << ":"
                << "m_WRTC->WRTCQueue.dispatch type == candidate";
          rapidjson::Document message_object1; // TODO
          message_object1.Parse(msgPayload.c_str()); // TODO
          m_WRTC->createAndAddIceCandidate(message_object1);
        });*/

  // send same message back (ping-pong)
  // clientSession->send(beast::buffers_to_string(messageBuffer->data()));
}

void offerCallback(WsSession* clientSession, NetworkManager* nm,
                   std::shared_ptr<beast::multi_buffer> messageBuffer) {
  if (!messageBuffer || !messageBuffer.get()) {
    LOG(WARNING) << "WsServer: Invalid messageBuffer";
    return;
  }

  if (!clientSession) {
    LOG(WARNING) << "WSServer invalid clientSession!";
    return;
  }

  const std::string incomingStr = beast::buffers_to_string(messageBuffer->data());
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "offerCallback incomingMsg=" << incomingStr;

  // todo: pass parsed
  rapidjson::Document message_object;
  auto msgPayload = beast::buffers_to_string(messageBuffer->data());
  message_object.Parse(msgPayload.c_str());
  LOG(INFO) << msgPayload.c_str();

  // TODO: don`t create datachennel for same client twice?
  LOG(INFO) << "type == offer";

  if (!clientSession->getWRTC()) {
    LOG(WARNING) << "WSServer::OnWebSocketMessage invalid m_WRTC!";
    return;
  }

  if (!clientSession->getWRTCQueue()) {
    LOG(WARNING) << "WSServer::OnWebSocketMessage invalid m_WRTC->WRTCQueue!";
    return;
  }

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "m_WRTC->WRTCQueue.dispatch type == offer";
  rapidjson::Document message_obj;       // TODO
  message_obj.Parse(msgPayload.c_str()); // TODO
  WRTCSession::setRemoteDescriptionAndCreateAnswer(clientSession, nm, message_obj);
  /*m_WRTC->WRTCQueue->dispatch([m_WRTC, msgPayload] {
          LOG(INFO) << std::this_thread::get_id() << ":"
                << "m_WRTC->WRTCQueue.dispatch type == offer";
          rapidjson::Document message_object1; // TODO
          message_object1.Parse(msgPayload.c_str()); // TODO
          m_WRTC->SetRemoteDescriptionAndCreateAnswer(message_object1);
        });*/
  LOG(INFO) << "added to WRTCQueue type == offer";

  // send same message back (ping-pong)
  // clientSession->send(beast::buffers_to_string(messageBuffer->data()));
}

// TODO: answerCallback unused
void answerCallback(WsSession* clientSession, NetworkManager* nm,
                    std::shared_ptr<beast::multi_buffer> messageBuffer) {
  if (!messageBuffer || !messageBuffer.get()) {
    LOG(WARNING) << "WsServer: Invalid messageBuffer";
    return;
  }

  if (!clientSession) {
    LOG(WARNING) << "WSServer invalid clientSession!";
    return;
  }

  const std::string incomingStr = beast::buffers_to_string(messageBuffer->data());
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "answerCallback incomingMsg=" << incomingStr;
  // send same message back (ping-pong)
  // clientSession->send(incomingStr);
}

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

WSInputCallbacks WSServer::getWsOperationCallbacks() const { return wsOperationCallbacks_; }

WSServer::WSServer(NetworkManager* nm) : nm_(nm) {
  const WsNetworkOperation PING_OPERATION =
      WsNetworkOperation(algo::WS_OPCODE::PING, algo::Opcodes::opcodeToStr(algo::WS_OPCODE::PING));
  wsOperationCallbacks_.addCallback(PING_OPERATION, &pingCallback);

  const WsNetworkOperation CANDIDATE_OPERATION = WsNetworkOperation(
      algo::WS_OPCODE::CANDIDATE, algo::Opcodes::opcodeToStr(algo::WS_OPCODE::CANDIDATE));
  wsOperationCallbacks_.addCallback(CANDIDATE_OPERATION, &candidateCallback);

  const WsNetworkOperation OFFER_OPERATION = WsNetworkOperation(
      algo::WS_OPCODE::OFFER, algo::Opcodes::opcodeToStr(algo::WS_OPCODE::OFFER));
  wsOperationCallbacks_.addCallback(OFFER_OPERATION, &offerCallback);

  const WsNetworkOperation ANSWER_OPERATION = WsNetworkOperation(
      algo::WS_OPCODE::ANSWER, algo::Opcodes::opcodeToStr(algo::WS_OPCODE::ANSWER));
  wsOperationCallbacks_.addCallback(ANSWER_OPERATION, &answerCallback);
}

/**
 * @brief calls game engine to interpret the message and sends a response to
 * clients
 *
 * @param id id of client who sent the message
 * @param message actual message
 */
/*void WSServer::interpret(size_t id, const std::string& message) {
  auto return_data = game_.doWork(id, message);
  for (auto& msg : return_data) {
    for (auto user_id : msg.second) {
      if (auto session = sessions_.at(user_id).lock()) {
        session->write(msg.first);
      } else {
        //throw std::runtime_error(
            "WsServer: trying to send data to non-existing session");
      }
    }
  }
}*/

/**
 * @brief removes session from list of valid sessions
 *
 * @param id id of session to be removed
 */
void WSServer::unregisterSession(const std::string& id) {
  {
    rtc::CritScope lock(&sessionsMutex_);
    std::shared_ptr<WsSession> sess = getSessById(id);
    if (!sessions_.erase(id)) {
      // throw std::runtime_error(
      LOG(WARNING) << "WsServer::unregisterSession: trying to unregister non-existing session";
      // NOTE: continue cleanup with saved shared_ptr
    }
    if (!sess) {
      // throw std::runtime_error(
      LOG(WARNING) << "WsServer::unregisterSession: session already deleted";
      return;
    }
    /*if (sess) {
      sess.reset();
    }*/
  }
  LOG(INFO) << "WsServer: unregistered " << id;
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
    // rtc::CritScope lock(&sessionsMutex_);
    for (auto& sessionkv : sessions_) {
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
    // rtc::CritScope lock(&sessionsMutex_);
    auto it = sessions_.find(sessionID);
    if (it != sessions_.end()) {
      if (!it->second || !it->second.get()) {
        LOG(WARNING) << "WSServer::sendTo: Invalid session ";
        return;
      }
      it->second->send(message);
    }
  }
}

/**
 * @example:
 * sm->doToAll([&](std::shared_ptr<utils::net::WsSession> session) {
 *   session.get()->send("Your id: " + session.get()->getId());
 * });
 **/
void WSServer::doToAllSessions(std::function<void(std::shared_ptr<WsSession>)> func) {
  {
    // rtc::CritScope lock(&sessionsMutex_);
    for (auto& sessionkv : sessions_) {
      if (auto session = sessionkv.second) {
        if (!session || !session.get()) {
          LOG(WARNING) << "doToAllSessions: Invalid session ";
          continue;
        }
        func(session); /// <<<<<<<<<<<<<<<<<<<<<
      }
    }
  }
}

size_t WSServer::getSessionsCount() const {
  // rtc::CritScope lock(&sessionsMutex_);
  return sessions_.size();
}

std::unordered_map<std::string, std::shared_ptr<WsSession>> WSServer::getSessions() const {
  // rtc::CritScope lock(&sessionsMutex_);
  return sessions_;
}

std::shared_ptr<WsSession> WSServer::getSessById(const std::string& sessionID) {
  {
    // rtc::CritScope lock(&sessionsMutex_);
    auto it = sessions_.find(sessionID);
    if (it != sessions_.end()) {
      return it->second;
    }
  }
  LOG(WARNING) << "WSServer::getSessById: unknown session with id = " << sessionID;
  return nullptr;
}

void WSServer::handleAllPlayerMessages() {
  doToAllSessions([&](std::shared_ptr<utils::net::WsSession> session) {
    if (!session) {
      LOG(WARNING) << "WsServer::handleAllPlayerMessages: trying to "
                      "use non-existing session";
      return;
    }
    session->getReceivedMessages()->DispatchQueued();
    // TODO
    // session->getReceivedMessages()->dispatch_loop();
  });
}

/**
 * @brief adds a session to list of valid sessions
 *
 * @param session session to be registered
 */
bool WSServer::addSession(const std::string& sessionID, std::shared_ptr<WsSession> sess) {
  if (!sess || !sess.get()) {
    LOG(WARNING) << "addSession: Invalid session ";
    return false;
  }
  {
    rtc::CritScope lock(&sessionsMutex_);
    sessions_[sessionID] = sess;
  }
  return true; // TODO: handle collision
}

} // namespace net
} // namespace utils

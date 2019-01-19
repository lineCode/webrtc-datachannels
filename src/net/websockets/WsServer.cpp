#include "net/websockets/WsServer.hpp" // IWYU pragma: associated
#include "algorithm/DispatchQueue.hpp"
#include "algorithm/NetworkOperation.hpp"
#include "log/Logger.hpp"
#include "net/webrtc/WRTCServer.hpp"
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

void pingCallback(WsSession* clientSession, std::shared_ptr<beast::multi_buffer> messageBuffer) {
  const std::string incomingStr = beast::buffers_to_string(messageBuffer->data());
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "pingCallback incomingMsg=" << incomingStr;
  // send same message back (ping-pong)
  clientSession->send(incomingStr);
}

void candidateCallback(WsSession* clientSession,
                       std::shared_ptr<beast::multi_buffer> messageBuffer) {
  const std::string incomingStr = beast::buffers_to_string(messageBuffer->data());
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "candidateCallback incomingMsg=" << incomingStr;

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
  }
  if (!clientSession->getWRTCQueue()) {
    LOG(WARNING) << "WSServer::OnWebSocketMessage invalid m_WRTC->WRTCQueue!";
  }
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "m_WRTC->WRTCQueue.dispatch type == candidate";
  rapidjson::Document message_object1;       // TODO
  message_object1.Parse(msgPayload.c_str()); // TODO
  clientSession->getWRTC()->createAndAddIceCandidate(message_object1);
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

void offerCallback(WsSession* clientSession, std::shared_ptr<beast::multi_buffer> messageBuffer) {
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
  }
  if (!clientSession->getWRTCQueue()) {
    LOG(WARNING) << "WSServer::OnWebSocketMessage invalid m_WRTC->WRTCQueue!";
  }
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "m_WRTC->WRTCQueue.dispatch type == offer";
  rapidjson::Document message_object1;       // TODO
  message_object1.Parse(msgPayload.c_str()); // TODO
  clientSession->getWRTC()->SetRemoteDescriptionAndCreateAnswer(message_object1);
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
void answerCallback(WsSession* clientSession, std::shared_ptr<beast::multi_buffer> messageBuffer) {
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

/**
 * @brief adds a session to list of valid sessions
 *
 * @param session session to be registered
 */
void WSServer::registerSession(const std::shared_ptr<WsSession>& session) {
  // maxSessionId_ = std::max(session->getId(), maxSessionId_);
  // const std::string wsGuid =
  // boost::lexical_cast<std::string>(session->getId());
  sessions_.insert(std::make_pair(session->getId(), session));
  LOG(INFO) << "total ws sessions: " << getSessionsCount();
}

WSInputCallbacks WSServer::getWsOperationCallbacks() const { return wsOperationCallbacks_; }

/**
 * @brief removes session from list of valid sessions
 *
 * @param id id of session to be removed
 */
void WSServer::unregisterSession(const std::string& id) {
  if (!sessions_.erase(id)) {
    // throw std::runtime_error(
    LOG(WARNING) << "WsServer: trying to unregister non-existing session";
  }
  LOG(INFO) << "WsServer: unregistered " << id;
}

WSServer::WSServer() {
  const WsNetworkOperation PING_OPERATION =
      WsNetworkOperation(WS_OPCODE::PING, Opcodes::opcodeToStr(WS_OPCODE::PING));
  wsOperationCallbacks_.addCallback(PING_OPERATION, &pingCallback);

  const WsNetworkOperation CANDIDATE_OPERATION =
      WsNetworkOperation(WS_OPCODE::CANDIDATE, Opcodes::opcodeToStr(WS_OPCODE::CANDIDATE));
  wsOperationCallbacks_.addCallback(CANDIDATE_OPERATION, &candidateCallback);

  const WsNetworkOperation OFFER_OPERATION =
      WsNetworkOperation(WS_OPCODE::OFFER, Opcodes::opcodeToStr(WS_OPCODE::OFFER));
  wsOperationCallbacks_.addCallback(OFFER_OPERATION, &offerCallback);

  const WsNetworkOperation ANSWER_OPERATION =
      WsNetworkOperation(WS_OPCODE::ANSWER, Opcodes::opcodeToStr(WS_OPCODE::ANSWER));
  wsOperationCallbacks_.addCallback(ANSWER_OPERATION, &answerCallback);
}

/**
 * @brief calls game engine to interpret the message and sends a response to
 * clients
 *
 * @param id id of client who sent the message
 * @param message actual message
 */
void WSServer::interpret(size_t id, const std::string& message) {
  /*auto return_data = game_.doWork(id, message);
  for (auto& msg : return_data) {
    for (auto user_id : msg.second) {
      if (auto session = sessions_.at(user_id).lock()) {
        session->write(msg.first);
      } else {
        //throw std::runtime_error(
            "WsServer: trying to send data to non-existing session");
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
void WSServer::sendToAll(const std::string& message) {
  LOG(WARNING) << "WSServer::sendToAll:" << message;
  for (auto& sessionkv : sessions_) {
    if (auto session = sessionkv.second.get()) {
      session->send(message);
    }
  }
}

void WSServer::sendTo(const std::string& sessionID, const std::string& message) {
  auto it = sessions_.find(sessionID);
  if (it != sessions_.end()) {
    it->second->send(message);
  }
}

/**
 * @example:
 * sm->doToAll([&](std::shared_ptr<utils::net::WsSession> session) {
 *   session.get()->send("Your id: " + session.get()->getId());
 * });
 **/
void WSServer::doToAllPlayers(std::function<void(std::shared_ptr<WsSession>)> func) {
  for (auto& sessionkv : sessions_) {
    if (auto session = sessionkv.second) {
      func(session);
    }
  }
}

void WSServer::handleAllPlayerMessages() {
  doToAllPlayers([&](std::shared_ptr<utils::net::WsSession> session) {
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

} // namespace net
} // namespace utils

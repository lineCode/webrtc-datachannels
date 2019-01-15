#include "net/NetworkManager.hpp" // IWYU pragma: associated
#include "net/WsSession.hpp"
#include "net/WsSessionManager.hpp"
#include <boost/beast/core/buffers_range.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <iostream>
#include <thread>

namespace utils {
namespace net {

const NetworkOperation PING_OPERATION = NetworkOperation(0, "PING");
const NetworkOperation CANDIDATE_OPERATION = NetworkOperation(1, "CANDIDATE");
const NetworkOperation OFFER_OPERATION = NetworkOperation(2, "OFFER");
const NetworkOperation ANSWER_OPERATION = NetworkOperation(3, "ANSWER");
// TODO: handle all opcodes

void pingCallback(WsSession* clientSession,
                  std::shared_ptr<beast::multi_buffer> messageBuffer) {
  const std::string incomingStr =
      beast::buffers_to_string(messageBuffer->data());
  std::cout << std::this_thread::get_id() << ":"
            << "pingCallback incomingMsg=" << incomingStr << std::endl;
  // send same message back (ping-pong)
  clientSession->send(beast::buffers_to_string(messageBuffer->data()));
}

void candidateCallback(WsSession* clientSession,
                       std::shared_ptr<beast::multi_buffer> messageBuffer) {
  const std::string incomingStr =
      beast::buffers_to_string(messageBuffer->data());
  std::cout << std::this_thread::get_id() << ":"
            << "candidateCallback incomingMsg=" << incomingStr << std::endl;
  // send same message back (ping-pong)
  clientSession->send(beast::buffers_to_string(messageBuffer->data()));
}

void offerCallback(WsSession* clientSession,
                   std::shared_ptr<beast::multi_buffer> messageBuffer) {
  const std::string incomingStr =
      beast::buffers_to_string(messageBuffer->data());
  std::cout << std::this_thread::get_id() << ":"
            << "offerCallback incomingMsg=" << incomingStr << std::endl;
  // send same message back (ping-pong)
  clientSession->send(beast::buffers_to_string(messageBuffer->data()));
}

void answerCallback(WsSession* clientSession,
                    std::shared_ptr<beast::multi_buffer> messageBuffer) {
  const std::string incomingStr =
      beast::buffers_to_string(messageBuffer->data());
  std::cout << std::this_thread::get_id() << ":"
            << "answerCallback incomingMsg=" << incomingStr << std::endl;
  // send same message back (ping-pong)
  clientSession->send(beast::buffers_to_string(messageBuffer->data()));
}

NetworkManager::NetworkManager() {
  sm_ = std::make_shared<utils::net::WsSessionManager>();
  wsOperationCallbacks_[PING_OPERATION] = &pingCallback;
  wsOperationCallbacks_[CANDIDATE_OPERATION] = &candidateCallback;
  wsOperationCallbacks_[OFFER_OPERATION] = &offerCallback;
  wsOperationCallbacks_[ANSWER_OPERATION] = &answerCallback;
}

std::shared_ptr<utils::net::WsSessionManager>
NetworkManager::getWsSessionManager() const {
  return sm_;
}

std::map<NetworkOperation, NetworkOperationCallback>
NetworkManager::getWsOperationCallbacks() const {
  return wsOperationCallbacks_;
}

} // namespace net
} // namespace utils

#include "net/NetworkManager.hpp" // IWYU pragma: associated
#include "log/Logger.hpp"
#include "net/WsSession.hpp"
#include "net/WsSessionManager.hpp"
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <thread>

namespace utils {
namespace net {

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

const NetworkOperation PING_OPERATION = NetworkOperation(0, "PING");
const NetworkOperation CANDIDATE_OPERATION = NetworkOperation(1, "CANDIDATE");
const NetworkOperation OFFER_OPERATION = NetworkOperation(2, "OFFER");
const NetworkOperation ANSWER_OPERATION = NetworkOperation(3, "ANSWER");
// TODO: handle all opcodes

void pingCallback(WsSession* clientSession,
                  std::shared_ptr<beast::multi_buffer> messageBuffer) {
  const std::string incomingStr =
      beast::buffers_to_string(messageBuffer->data());
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "pingCallback incomingMsg=" << incomingStr;
  // send same message back (ping-pong)
  clientSession->send(beast::buffers_to_string(messageBuffer->data()));
}

void candidateCallback(WsSession* clientSession,
                       std::shared_ptr<beast::multi_buffer> messageBuffer) {
  const std::string incomingStr =
      beast::buffers_to_string(messageBuffer->data());
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "candidateCallback incomingMsg=" << incomingStr;
  // send same message back (ping-pong)
  // clientSession->send(beast::buffers_to_string(messageBuffer->data()));
}

void offerCallback(WsSession* clientSession,
                   std::shared_ptr<beast::multi_buffer> messageBuffer) {
  const std::string incomingStr =
      beast::buffers_to_string(messageBuffer->data());
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "offerCallback incomingMsg=" << incomingStr;
  // send same message back (ping-pong)
  // clientSession->send(beast::buffers_to_string(messageBuffer->data()));
}

void answerCallback(WsSession* clientSession,
                    std::shared_ptr<beast::multi_buffer> messageBuffer) {
  const std::string incomingStr =
      beast::buffers_to_string(messageBuffer->data());
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "answerCallback incomingMsg=" << incomingStr;
  // send same message back (ping-pong)
  // clientSession->send(beast::buffers_to_string(messageBuffer->data()));
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

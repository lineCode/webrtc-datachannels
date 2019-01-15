#include "net/NetworkManager.hpp" // IWYU pragma: associated
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/WsListener.hpp"
#include "net/WsSession.hpp"
#include "net/WsSessionManager.hpp"
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <filesystem>
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

NetworkManager::NetworkManager(const utils::config::ServerConfig& serverConfig)
    : ioc_(serverConfig.threads_) {
  wssm_ = std::make_shared<utils::net::WsSessionManager>();
  wsOperationCallbacks_[PING_OPERATION] = &pingCallback;
  wsOperationCallbacks_[CANDIDATE_OPERATION] = &candidateCallback;
  wsOperationCallbacks_[OFFER_OPERATION] = &offerCallback;
  wsOperationCallbacks_[ANSWER_OPERATION] = &answerCallback;
}

std::shared_ptr<utils::net::WsSessionManager>
NetworkManager::getWsSessionManager() const {
  return wssm_;
}

std::map<NetworkOperation, NetworkOperationCallback>
NetworkManager::getWsOperationCallbacks() const {
  return wsOperationCallbacks_;
}

void NetworkManager::handleAllPlayerMessages() {
  wssm_->handleAllPlayerMessages();
  // TODO: wrtc->handleAllPlayerMessages();
}

void NetworkManager::runWsThreads(
    const utils::config::ServerConfig& serverConfig) {
  wsThreads_.reserve(serverConfig.threads_);
  for (auto i = serverConfig.threads_; i > 0; --i) {
    wsThreads_.emplace_back([this] { ioc_.run(); });
  }
}

void NetworkManager::finishWsThreads() {
  // Block until all the threads exit
  for (auto& t : wsThreads_) {
    if (t.joinable()) {
      t.join();
    }
  }
}

void NetworkManager::runIocWsListener(
    const utils::config::ServerConfig& serverConfig) {

  const tcp::endpoint tcpEndpoint =
      tcp::endpoint{serverConfig.address_, serverConfig.port_};

  utils::net::NetworkManager* nm = this;

  std::shared_ptr<std::string const> workdirPtr =
      std::make_shared<std::string>(serverConfig.workdir_.string());

  /*
  WsListener(boost::asio::io_context& ioc,
             boost::asio::ip::tcp::endpoint endpoint,
             std::shared_ptr<std::string const> const& doc_root,
             utils::net::NetworkManager* nm)
*/

  // Create and launch a listening port
  const std::shared_ptr<utils::net::WsListener> iocWsListener =
      std::make_shared<utils::net::WsListener>(ioc_, tcpEndpoint, workdirPtr,
                                               nm);
  /*const std::shared_ptr<utils::net::WsListener> iocWsListener =
      std::make_shared<utils::net::WsListener>(ioc_);*/

  iocWsListener->run();
}

} // namespace net
} // namespace utils

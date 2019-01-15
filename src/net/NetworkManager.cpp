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

// TODO: handle all opcodes

#ifdef NOPE
/*
 * Callback for when the WebSocket server receives a message from the client.
 * The peer connection needs 2 things from the server to set up the WebRTC
 *connection: 1 a session description 2 ice candidates. We get this info through
 *webrtc messages.
 **/
void WSServer::OnWebSocketMessage(WRTCServer* m_WRTC, WSServer* m_WS,
                                  WebSocketServer* /* s */,
                                  websocketpp::connection_hdl hdl,
                                  message_ptr msg) {
  std::cout << std::this_thread::get_id() << ":"
            << "WSServer::OnWebSocketMessage" << std::endl;
  m_WS->websocket_connection_handler = hdl;
  const std::string msgPayload = msg->get_payload();
  rapidjson::Document message_object;
  message_object.Parse(msgPayload.c_str());
  std::cout << msgPayload.c_str() << std::endl;
  // Probably should do some error checking on the JSON object.
  std::string type = message_object["type"].GetString();
  if (type == "ping") {
    m_WS->handleWebsocketsPing(hdl, msg);
  } else if (type == "offer") {
    // TODO: don`t create datachennel for same client twice?
    std::cout << "type == offer" << std::endl;
    if (!m_WRTC) {
      std::cout << "WSServer::OnWebSocketMessage invalid m_WRTC!" << std::endl;
    }
    if (!m_WRTC->WRTCQueue) {
      std::cout << "WSServer::OnWebSocketMessage invalid m_WRTC->WRTCQueue!"
                << std::endl;
    }
    std::cout << std::this_thread::get_id() << ":"
              << "m_WRTC->WRTCQueue.dispatch type == offer" << std::endl;
    rapidjson::Document message_object1;       // TODO
    message_object1.Parse(msgPayload.c_str()); // TODO
    m_WRTC->SetRemoteDescriptionAndCreateAnswer(message_object1);
    /*m_WRTC->WRTCQueue->dispatch([m_WRTC, msgPayload] {
          std::cout << std::this_thread::get_id() << ":"
                << "m_WRTC->WRTCQueue.dispatch type == offer" << std::endl;
          rapidjson::Document message_object1; // TODO
          message_object1.Parse(msgPayload.c_str()); // TODO
          m_WRTC->SetRemoteDescriptionAndCreateAnswer(message_object1);
        });*/
    std::cout << "added to WRTCQueue type == offer" << std::endl;
  } else if (type == "candidate") {
    if (!m_WRTC) {
      std::cout << "WSServer::OnWebSocketMessage invalid m_WRTC!" << std::endl;
    }
    if (!m_WRTC->WRTCQueue) {
      std::cout << "WSServer::OnWebSocketMessage invalid m_WRTC->WRTCQueue!"
                << std::endl;
    }
    // Server receives Client’s ICE candidates, then finds its own ICE
    // candidates & sends them to Client
    std::cout << "type == candidate" << std::endl;
    if (!m_WRTC) {
      std::cout << "WSServer::OnWebSocketMessage invalid m_WRTC!" << std::endl;
    }
    if (!m_WRTC->WRTCQueue) {
      std::cout << "WSServer::OnWebSocketMessage invalid m_WRTC->WRTCQueue!"
                << std::endl;
    }
    std::cout << std::this_thread::get_id() << ":"
              << "m_WRTC->WRTCQueue.dispatch type == candidate" << std::endl;
    rapidjson::Document message_object1;       // TODO
    message_object1.Parse(msgPayload.c_str()); // TODO
    m_WRTC->createAndAddIceCandidate(message_object1);
    /*m_WRTC->WRTCQueue->dispatch([m_WRTC, msgPayload] {
          std::cout << std::this_thread::get_id() << ":"
                << "m_WRTC->WRTCQueue.dispatch type == candidate" << std::endl;
          rapidjson::Document message_object1; // TODO
          message_object1.Parse(msgPayload.c_str()); // TODO
          m_WRTC->createAndAddIceCandidate(message_object1);
        });*/
    std::cout << "added to WRTCQueue type == candidate" << std::endl;
  } else {
    std::cout << "Unrecognized WebSocket message type." << std::endl;
  }
}
#endif

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

  // todo: pass parsed
  rapidjson::Document message_object;
  auto msgPayload = beast::buffers_to_string(messageBuffer->data());
  message_object.Parse(msgPayload.c_str());
  std::cout << msgPayload.c_str() << std::endl;

  if (!clientSession->nm_->getWrtc()) {
    std::cout << "WSServer::OnWebSocketMessage invalid m_WRTC!" << std::endl;
  }
  if (!clientSession->nm_->getWrtc()->WRTCQueue) {
    std::cout << "WSServer::OnWebSocketMessage invalid m_WRTC->WRTCQueue!"
              << std::endl;
  }
  // Server receives Client’s ICE candidates, then finds its own ICE
  // candidates & sends them to Client
  std::cout << "type == candidate" << std::endl;
  if (!clientSession->nm_->getWrtc()) {
    std::cout << "WSServer::OnWebSocketMessage invalid m_WRTC!" << std::endl;
  }
  if (!clientSession->nm_->getWrtc()->WRTCQueue) {
    std::cout << "WSServer::OnWebSocketMessage invalid m_WRTC->WRTCQueue!"
              << std::endl;
  }
  std::cout << std::this_thread::get_id() << ":"
            << "m_WRTC->WRTCQueue.dispatch type == candidate" << std::endl;
  rapidjson::Document message_object1;       // TODO
  message_object1.Parse(msgPayload.c_str()); // TODO
  clientSession->nm_->getWrtc()->createAndAddIceCandidate(message_object1);
  /*m_WRTC->WRTCQueue->dispatch([m_WRTC, msgPayload] {
          std::cout << std::this_thread::get_id() << ":"
                << "m_WRTC->WRTCQueue.dispatch type == candidate" << std::endl;
          rapidjson::Document message_object1; // TODO
          message_object1.Parse(msgPayload.c_str()); // TODO
          m_WRTC->createAndAddIceCandidate(message_object1);
        });*/

  // send same message back (ping-pong)
  // clientSession->send(beast::buffers_to_string(messageBuffer->data()));
}

void offerCallback(WsSession* clientSession,
                   std::shared_ptr<beast::multi_buffer> messageBuffer) {
  const std::string incomingStr =
      beast::buffers_to_string(messageBuffer->data());
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "offerCallback incomingMsg=" << incomingStr;

  // todo: pass parsed
  rapidjson::Document message_object;
  auto msgPayload = beast::buffers_to_string(messageBuffer->data());
  message_object.Parse(msgPayload.c_str());
  std::cout << msgPayload.c_str() << std::endl;

  // TODO: don`t create datachennel for same client twice?
  std::cout << "type == offer" << std::endl;
  if (!clientSession->nm_->getWrtc()) {
    std::cout << "WSServer::OnWebSocketMessage invalid m_WRTC!" << std::endl;
  }
  if (!clientSession->nm_->getWrtc()->WRTCQueue) {
    std::cout << "WSServer::OnWebSocketMessage invalid m_WRTC->WRTCQueue!"
              << std::endl;
  }
  std::cout << std::this_thread::get_id() << ":"
            << "m_WRTC->WRTCQueue.dispatch type == offer" << std::endl;
  rapidjson::Document message_object1;       // TODO
  message_object1.Parse(msgPayload.c_str()); // TODO
  clientSession->nm_->getWrtc()->SetRemoteDescriptionAndCreateAnswer(
      message_object1);
  /*m_WRTC->WRTCQueue->dispatch([m_WRTC, msgPayload] {
          std::cout << std::this_thread::get_id() << ":"
                << "m_WRTC->WRTCQueue.dispatch type == offer" << std::endl;
          rapidjson::Document message_object1; // TODO
          message_object1.Parse(msgPayload.c_str()); // TODO
          m_WRTC->SetRemoteDescriptionAndCreateAnswer(message_object1);
        });*/
  std::cout << "added to WRTCQueue type == offer" << std::endl;

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
  wrtcServer_ = std::make_shared<utils::net::WRTCServer>(this);
}

// The thread entry point for the WebRTC thread. This sets the WebRTC thread as
// the signaling thread and creates a worker thread in the background.
void NetworkManager::webRtcSignalThreadEntry(/*
    const utils::config::ServerConfig& serverConfig*/) {
  // ICE is the protocol chosen for NAT traversal in WebRTC.
  webrtc::PeerConnectionInterface::IceServer ice_servers[5];
  // TODO to ServerConfig + username/password
  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  ice_servers[0].uri = "stun:stun.l.google.com:19302";
  ice_servers[1].uri = "stun:stun1.l.google.com:19302";
  ice_servers[2].uri = "stun:stun2.l.google.com:19305";
  ice_servers[3].uri = "stun:stun01.sipphone.com";
  ice_servers[4].uri = "stun:stunserver.org";
  // TODO ice_server.username = "xxx";
  // TODO ice_server.password = kTurnPassword;
  // TODO
  // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  wrtcServer_->resetWebRtcConfig({ice_servers[0], ice_servers[1],
                                  ice_servers[2], ice_servers[3],
                                  ice_servers[4]});
  wrtcServer_->InitAndRun();
}

std::shared_ptr<utils::net::WsSessionManager>
NetworkManager::getWsSessionManager() const {
  return wssm_;
}

std::map<NetworkOperation, WsNetworkOperationCallback>
NetworkManager::getWsOperationCallbacks() const {
  return wsOperationCallbacks_;
}

void NetworkManager::handleAllPlayerMessages() {
  wssm_->handleAllPlayerMessages();
  // TODO: wrtc->handleAllPlayerMessages();
}

/*
 * TODO
#include <csignal>
/// Block until SIGINT or SIGTERM is received.
void sigWait(net::io_context& ioc) {
  // Capture SIGINT and SIGTERM to perform a clean shutdown
  boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
  signals.async_wait([&](boost::system::error_code const&, int) {
    // Stop the `io_context`. This will cause `run()`
    // to return immediately, eventually destroying the
    // `io_context` and all of the sockets in it.
    LOG(WARNING) << "Called ioc.stop() on SIGINT or SIGTERM";
    ioc.stop();
    doServerRun = false;
  });
}
*/

void NetworkManager::runWsThreads(
    const utils::config::ServerConfig& serverConfig) {
  wsThreads_.reserve(serverConfig.threads_);
  for (auto i = serverConfig.threads_; i > 0; --i) {
    wsThreads_.emplace_back([this] { ioc_.run(); });
  }
  // TODO sigWait(ioc);
  // TODO ioc.run();
}

void NetworkManager::runWrtcThreads(
    const utils::config::ServerConfig& serverConfig) {
  /*webrtc_thread_ = std::thread(&NetworkManager::webRtcSignalThreadEntry,
                               webRtcSignalThreadEntry());*/
  webrtc_thread_ = std::thread(&NetworkManager::webRtcSignalThreadEntry, this);
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

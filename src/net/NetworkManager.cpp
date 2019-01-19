#include "net/NetworkManager.hpp" // IWYU pragma: associated
#include "config/ServerConfig.hpp"
#include "net/webrtc/WRTCServer.hpp"
#include "net/websockets/WsListener.hpp"
#include "net/websockets/WsServer.hpp"
#include <api/peerconnectioninterface.h>
#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <filesystem>
#include <string>
#include <thread>

namespace utils {
namespace net {

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

NetworkManager::NetworkManager(const utils::config::ServerConfig& serverConfig)
    : ioc_(serverConfig.threads_) {
  wsServer_ = std::make_shared<utils::net::WSServer>();
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
  wrtcServer_->resetWebRtcConfig(
      {ice_servers[0], ice_servers[1], ice_servers[2], ice_servers[3], ice_servers[4]});
  wrtcServer_->InitAndRun();
}

std::shared_ptr<utils::net::WRTCServer> NetworkManager::getWRTC() const { return wrtcServer_; }

std::shared_ptr<utils::net::WSServer> NetworkManager::getWS() const { return wsServer_; }

void NetworkManager::handleAllPlayerMessages() {
  wsServer_->handleAllPlayerMessages();
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

void NetworkManager::run(const utils::config::ServerConfig& serverConfig) {
  runIocWsListener(serverConfig);

  // TODO int max_thread_num = std::thread::hardware_concurrency();

  runWsThreads(serverConfig);

  runWrtcThreads(serverConfig);
}

void NetworkManager::finish() {
  finishWsThreads();
  // TODO finishWrtcThreads();
}

void NetworkManager::runWsThreads(const utils::config::ServerConfig& serverConfig) {
  wsThreads_.reserve(serverConfig.threads_);
  for (auto i = serverConfig.threads_; i > 0; --i) {
    wsThreads_.emplace_back([this] { ioc_.run(); });
  }
  // TODO sigWait(ioc);
  // TODO ioc.run();
}

void NetworkManager::runWrtcThreads(const utils::config::ServerConfig& serverConfig) {
  /*webrtc_thread_ = std::thread(&NetworkManager::webRtcSignalThreadEntry,
                               webRtcSignalThreadEntry());*/
  webrtcThread_ = std::thread(&NetworkManager::webRtcSignalThreadEntry, this);
}

void NetworkManager::finishWsThreads() {
  // Block until all the threads exit
  for (auto& t : wsThreads_) {
    if (t.joinable()) {
      t.join();
    }
  }
}

void NetworkManager::runIocWsListener(const utils::config::ServerConfig& serverConfig) {

  const tcp::endpoint tcpEndpoint = tcp::endpoint{serverConfig.address_, serverConfig.wsPort_};

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
      std::make_shared<utils::net::WsListener>(ioc_, tcpEndpoint, workdirPtr, nm);
  /*const std::shared_ptr<utils::net::WsListener> iocWsListener =
      std::make_shared<utils::net::WsListener>(ioc_);*/

  iocWsListener->run();
}

} // namespace net
} // namespace utils

#include "net/NetworkManager.hpp" // IWYU pragma: associated
#include "config/ServerConfig.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/ws/WsListener.hpp"
#include "net/ws/WsServer.hpp"
#include <api/peerconnectioninterface.h>
#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <filesystem>
#include <string>
#include <thread>

namespace gloer {
namespace net {

using namespace gloer::net::ws;
using namespace gloer::net::wrtc;

NetworkManager::NetworkManager(const gloer::config::ServerConfig& serverConfig) {}

std::shared_ptr<WRTCServer> NetworkManager::getWRTC() const { return wrtcServer_; }

std::shared_ptr<WSServer> NetworkManager::getWS() const { return wsServer_; }

/*
 * TODO
#include <csignal>
/// Block until SIGINT or SIGTERM is received.
void sigWait(::net::io_context& ioc) {
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

void NetworkManager::initServers(const gloer::config::ServerConfig& serverConfig) {
  // NOTE: no 'this' in constructor
  wsServer_ = std::make_shared<WSServer>(this, serverConfig);
  wrtcServer_ = std::make_shared<WRTCServer>(this, serverConfig);
}

void NetworkManager::startServers(const gloer::config::ServerConfig& serverConfig) {

  // TODO int max_thread_num = std::thread::hardware_concurrency();

  wsServer_->runThreads_t(serverConfig);
  wrtcServer_->runThreads_t(serverConfig);
}

void NetworkManager::finishServers() {
  wsServer_->getListener()->stop();
  wsServer_->finishThreads_t();
  wrtcServer_->finishThreads_t();
}

void NetworkManager::runAsServer(const gloer::config::ServerConfig& serverConfig) {
  initServers(serverConfig);

  wsServer_->runAsServer(serverConfig);

  startServers(serverConfig);
}

void NetworkManager::runAsClient(const gloer::config::ServerConfig& serverConfig) {
  initServers(serverConfig);

  wsServer_->runAsClient(serverConfig);

  startServers(serverConfig);
}

} // namespace net
} // namespace gloer

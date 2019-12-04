#include "net/NetworkManager.hpp" // IWYU pragma: associated
#include "config/ServerConfig.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/ws/WsListener.hpp"
#include "net/ws/WsServer.hpp"
#include "net/ws/client/Client.hpp"
#include <api/peerconnectioninterface.h>
#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>

#ifndef __has_include
  static_assert(false, "__has_include not supported");
#else
#  if __has_include(<filesystem>)
#    include <filesystem>
     namespace fs = std::filesystem;
#  elif __has_include(<experimental/filesystem>)
#    include <experimental/filesystem>
     namespace fs = std::experimental::filesystem;
#  elif __has_include(<boost/filesystem.hpp>)
#    include <boost/filesystem.hpp>
     namespace fs = boost::filesystem;
#  endif
#endif

#include <string>
#include <thread>

namespace gloer {
namespace net {

using namespace gloer::net::ws;
using namespace gloer::net::wrtc;

NetworkManager::NetworkManager(const gloer::config::ServerConfig& serverConfig)
  : ws_sm_(this), wrtc_sm_(this)
{}

std::shared_ptr<WRTCServer> NetworkManager::getWRTC() const { return wrtcServer_; }

std::shared_ptr<WSServer> NetworkManager::getWS() const { return wsServer_; }

std::shared_ptr<gloer::net::ws::Client> NetworkManager::getWSClient() const {
  return wsClient_;
}

/*
 * TODO
#include <csignal>
/// Block until SIGINT or SIGTERM is received.
void sigWait(::boost::asio::io_context& ioc) {
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
  wsServer_ = std::make_shared<WSServer>(this, serverConfig, ws_sm_);
  wrtcServer_ = std::make_shared<WRTCServer>(this, serverConfig, wrtc_sm_);
  wsClient_ = std::make_shared<gloer::net::ws::Client>(this, serverConfig, ws_sm_);
}

void NetworkManager::startServers(const gloer::config::ServerConfig& serverConfig) {

  // TODO int max_thread_num = std::thread::hardware_concurrency();

  RTC_DCHECK(wsServer_);
  wsServer_->runThreads_t(serverConfig);
  RTC_DCHECK(wrtcServer_);
  wrtcServer_->runThreads_t(serverConfig);
  RTC_DCHECK(wsClient_);
  wsClient_->runThreads_t(serverConfig);
}

void NetworkManager::finishServers() {
  LOG(WARNING) << "NetworkManager: finishing threads";
  RTC_DCHECK(wsClient_);
  wsClient_->finishThreads_t();
  RTC_DCHECK(wsServer_);
  if(wsServer_->getListener()) {
    RTC_DCHECK(wsServer_->getListener());
    wsServer_->getListener()->stop();
  }
  wsServer_->finishThreads_t();
  RTC_DCHECK(wrtcServer_);
  wrtcServer_->finishThreads_t();
}

void NetworkManager::runAsServer(const gloer::config::ServerConfig& serverConfig) {
  initServers(serverConfig);

  RTC_DCHECK(wsServer_);
  wsServer_->runAsServer(serverConfig);

  startServers(serverConfig);
}

void NetworkManager::runAsClient(const gloer::config::ServerConfig& serverConfig) {
  initServers(serverConfig);

  RTC_DCHECK(wsServer_);
  RTC_DCHECK(wsClient_);

#if 0
  wsServer_->runAsClient(serverConfig);
#endif // 0

  startServers(serverConfig);
}

} // namespace net
} // namespace gloer

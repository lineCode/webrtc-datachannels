#pragma once

#include <net/core.hpp>
#include <thread>
#include <vector>

namespace gloer {
namespace config {
struct ServerConfig;
} // namespace config
} // namespace gloer

namespace gloer {
namespace net {

namespace ws {
class WSServer;
class Client;
class ClientSessionManager;
class ServerSessionManager;
class WSClientInputCallbacks;
class WSServerInputCallbacks;
}

namespace wrtc {
class WRTCServer;
class SessionManager;
class WRTCInputCallbacks;
}

} // namespace net
} // namespace gloer

namespace gloer {
namespace net {

/*
class PlayerSession {
  PlayerSession() {}

  std::shared_ptr<WsSession> wsSess_;

  std::shared_ptr<WrtcSession> wrtcSess_;
};

class Player {
  Player() {}
  PlayerSession session;
};*/

class NetworkManagerBase {
public:
  virtual ~NetworkManagerBase() {}
  virtual void run(const gloer::config::ServerConfig& serverConfig) = 0;
  virtual void prepare(const gloer::config::ServerConfig& serverConfig) = 0;
  virtual void finish() = 0;
  //virtual std::shared_ptr<session_runner> getRunner() = 0;
};

template<
  typename session_runner,
  typename session_manager,
  typename operation_callbacks
>
class NetworkManager : public NetworkManagerBase {
public:
  explicit NetworkManager(const gloer::config::ServerConfig& serverConfig)
    : sm_(this) {
      // NOTE: no 'this' in constructor
      sessionRunner_ = std::make_shared<session_runner>(this, serverConfig, sm_);
    }

  void prepare(const gloer::config::ServerConfig& serverConfig) override {
    RTC_DCHECK(sessionRunner_);
    sessionRunner_->prepare(serverConfig);
  }

  void run(const gloer::config::ServerConfig& serverConfig) override {
    RTC_DCHECK(sessionRunner_);
    sessionRunner_->runThreads_t(serverConfig);
  }

  void finish() override {
    RTC_DCHECK(sessionRunner_);
    sessionRunner_->finishThreads_t();
  }

  std::shared_ptr<session_runner> getRunner() const { return sessionRunner_; }

  session_manager& sessionManager() {
    return sm_;
  }

  operation_callbacks& operationCallbacks() { return operationCallbacks_; }

private:
  std::shared_ptr<session_runner> sessionRunner_;

  session_manager sm_;

  operation_callbacks operationCallbacks_{};
};

typedef
  ::gloer::net::NetworkManager<
    ws::WSServer, ws::ServerSessionManager, ws::WSServerInputCallbacks
  > WSServerNetworkManager;

typedef
  ::gloer::net::NetworkManager<
    ws::Client, ws::ClientSessionManager, ws::WSClientInputCallbacks
  > WSClientNetworkManager;

typedef
  ::gloer::net::NetworkManager<
    wrtc::WRTCServer, wrtc::SessionManager, wrtc::WRTCInputCallbacks
  > WRTCNetworkManager;

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

} // namespace net
} // namespace gloer

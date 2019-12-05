#pragma once

#include <memory>

namespace gloer {
namespace config {
struct ServerConfig;
} // namespace config
} // namespace gloer

namespace gloer {
namespace net {

namespace ws {
class ServerConnectionManager;
class ClientConnectionManager;
class ClientSessionManager;
class ServerSessionManager;
class ClientInputCallbacks;
class ServerInputCallbacks;
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

class NetworkManagerBase {
public:
  virtual ~NetworkManagerBase() {}
  virtual void run(const gloer::config::ServerConfig& serverConfig) = 0;
  virtual void prepare(const gloer::config::ServerConfig& serverConfig) = 0;
  virtual void finish() = 0;
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
    sessionRunner_->run(serverConfig);
  }

  void finish() override {
    RTC_DCHECK(sessionRunner_);
    sessionRunner_->finish();
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
    ws::ServerConnectionManager, ws::ServerSessionManager, ws::ServerInputCallbacks
  > WSServerNetworkManager;

typedef
  ::gloer::net::NetworkManager<
    ws::ClientConnectionManager, ws::ClientSessionManager, ws::ClientInputCallbacks
  > WSClientNetworkManager;

typedef
  ::gloer::net::NetworkManager<
    wrtc::WRTCServer, wrtc::SessionManager, wrtc::WRTCInputCallbacks
  > WRTCNetworkManager;

} // namespace net
} // namespace gloer

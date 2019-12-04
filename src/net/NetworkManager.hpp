#pragma once

#include <net/core.hpp>
#include "net/ws/SessionManager.hpp"
#include "net/wrtc/SessionManager.hpp"
#include "net/ws/Callbacks.hpp"
#include "net/wrtc/Callbacks.hpp"
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
}

namespace wrtc {
class WRTCServer;
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

class NetworkManager {
public:
  explicit NetworkManager(const gloer::config::ServerConfig& serverConfig);

  void runAsServer(const gloer::config::ServerConfig& serverConfig);

  void finishServers();

  std::shared_ptr<wrtc::WRTCServer> getWRTC() const;

  std::shared_ptr<ws::WSServer> getWS() const;

  std::shared_ptr<ws::Client> getWSClient() const;

  wrtc::SessionManager& getWRTC_SM() {
    return wrtc_sm_;
  }

  ws::SessionManager& getWS_SM() {
    return ws_sm_;
  }

  void runAsClient(const gloer::config::ServerConfig& serverConfig);

  wrtc::WRTCInputCallbacks& getWRTCOperationCallbacks() { return wrtc_operationCallbacks_; }

  ws::WSInputCallbacks& getWSOperationCallbacks() { return ws_operationCallbacks_; }

private:
  std::shared_ptr<wrtc::WRTCServer> wrtcServer_;

  std::shared_ptr<ws::WSServer> wsServer_;

  std::shared_ptr<gloer::net::ws::Client> wsClient_;

  ws::SessionManager ws_sm_;

  wrtc::SessionManager wrtc_sm_;

  wrtc::WRTCInputCallbacks wrtc_operationCallbacks_{};

  ws::WSInputCallbacks ws_operationCallbacks_{};

  // TODO
  //  std::vector<Player> players_;

  void initServers(const gloer::config::ServerConfig& serverConfig);

  void startServers(const gloer::config::ServerConfig& serverConfig);
};

} // namespace net
} // namespace gloer

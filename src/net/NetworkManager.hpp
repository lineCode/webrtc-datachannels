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
  NetworkManager(const gloer::config::ServerConfig& serverConfig);

  void handleIncomingMessages();

  void runAsServer(const gloer::config::ServerConfig& serverConfig);

  void finishServers();

  std::shared_ptr<wrtc::WRTCServer> getWRTC() const;

  std::shared_ptr<ws::WSServer> getWS() const;

  void runAsClient(const gloer::config::ServerConfig& serverConfig);

private:
  std::shared_ptr<wrtc::WRTCServer> wrtcServer_;

  std::shared_ptr<ws::WSServer> wsServer_;

  // TODO
  //  std::vector<Player> players_;

  void initServers(const gloer::config::ServerConfig& serverConfig);

  void startServers(const gloer::config::ServerConfig& serverConfig);
};

} // namespace net
} // namespace gloer

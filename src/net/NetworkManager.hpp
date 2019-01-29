#pragma once

#include <net/core.hpp>
#include <thread>
#include <vector>

namespace gloer {
namespace config {
class ServerConfig;
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

/**
 * Type field stores uint32_t -> [0-4294967295] -> max 10 digits
 **/
constexpr unsigned long UINT32_FIELD_MAX_LEN = 10;

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

  void run(const gloer::config::ServerConfig& serverConfig);

  void finish();

  std::shared_ptr<wrtc::WRTCServer> getWRTC() const;

  std::shared_ptr<ws::WSServer> getWS() const;

private:
  std::shared_ptr<wrtc::WRTCServer> wrtcServer_;

  std::shared_ptr<ws::WSServer> wsServer_;

  // TODO
  //  std::vector<Player> players_;
};

} // namespace net
} // namespace gloer

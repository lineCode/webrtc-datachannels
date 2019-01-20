#pragma once

#include <boost/asio.hpp>
#include <thread>
#include <vector>

namespace utils {
namespace config {
class ServerConfig;
} // namespace config
} // namespace utils

namespace utils {
namespace net {

/**
 * Type field stores uint32_t -> [0-4294967295] -> max 10 digits
 **/
constexpr unsigned long UINT32_FIELD_MAX_LEN = 10;

class WSServer;
class WRTCServer;

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
  NetworkManager(const utils::config::ServerConfig& serverConfig);

  void handleAllPlayerMessages();

  void run(const utils::config::ServerConfig& serverConfig);

  void finish();

  std::shared_ptr<WRTCServer> getWRTC() const;

  std::shared_ptr<WSServer> getWS() const;

private:
  std::shared_ptr<WSServer> wsServer_;

  std::shared_ptr<WRTCServer> wrtcServer_;

  // TODO
  //  std::vector<Player> players_;
};

} // namespace net
} // namespace utils

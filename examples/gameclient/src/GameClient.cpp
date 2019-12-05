#include "GameClient.hpp" // IWYU pragma: associated

#include "WRTCServerManager.hpp"
#include "WSServerManager.hpp"
#include "config/ServerConfig.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/ws/server/ServerConnectionManager.hpp"
#include <api/peerconnectioninterface.h>
#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include "net/ws/client/ClientSessionManager.hpp"
#include "net/wrtc/SessionManager.hpp"
#include "net/ws/client/ClientConnectionManager.hpp"

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

namespace gameclient {

std::shared_ptr<
  ::gloer::net::WSClientNetworkManager
> GameClient::ws_nm = nullptr;

std::shared_ptr<
  ::gloer::net::WRTCNetworkManager
> GameClient::wrtc_nm = nullptr;

void GameClient::init(std::weak_ptr<GameClient> game) {
  wsGameManager = std::make_shared<WSServerManager>(game);
  wrtcGameManager = std::make_shared<WRTCServerManager>(game);
}

void GameClient::handleIncomingMessages() {
  // LOG(INFO) << "WS handleIncomingMessages";

  wsGameManager->processIncomingMessages();

  // LOG(INFO) << "WRTC handleIncomingMessages";
  wrtcGameManager->processIncomingMessages();
}

} // namespace gameclient

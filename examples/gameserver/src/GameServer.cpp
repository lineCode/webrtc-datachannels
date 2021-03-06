#include "GameServer.hpp" // IWYU pragma: associated

#include "WRTCServerManager.hpp"
#include "WSServerManager.hpp"
#include "config/ServerConfig.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include <api/peerconnectioninterface.h>
#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include "net/ws/server/ServerSessionManager.hpp"
#include "net/ws/server/Listener.hpp"
#include "net/wrtc/SessionManager.hpp"
#include "net/ws/server/ServerConnectionManager.hpp"

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

namespace gameserver {

std::shared_ptr<
  gloer::net::WSServerNetworkManager
> GameServer::ws_nm = nullptr;

std::shared_ptr<
  gloer::net::WRTCNetworkManager
> GameServer::wrtc_nm = nullptr;

void GameServer::init(std::weak_ptr<GameServer> game) {
  wsGameManager = std::make_shared<WSServerManager>(game);
  wrtcGameManager = std::make_shared<WRTCServerManager>(game);
}

void GameServer::handleIncomingMessages() {
  // LOG(INFO) << "WS handleIncomingMessages";

  if (!ws_nm->getRunner()->getListener()->isAccepting()) {
    LOG(WARNING) << "ws_nm iocWsListener_ not accepting incoming messages";
  } else {
    wsGameManager->processIncomingMessages();
  }

  // LOG(INFO) << "WRTC handleIncomingMessages";
  wrtcGameManager->processIncomingMessages();
}

} // namespace gameserver

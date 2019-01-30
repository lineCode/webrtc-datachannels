#include "GameServer.hpp" // IWYU pragma: associated

#include "WRTCServerManager.hpp"
#include "WSServerManager.hpp"
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

namespace gameserver {

std::shared_ptr<::gloer::net::NetworkManager> GameServer::nm = nullptr;

void GameServer::init(std::weak_ptr<GameServer> game) {
  wsGameManager = std::make_shared<WSServerManager>(game);
  wrtcGameManager = std::make_shared<WRTCServerManager>(game);
}

void GameServer::handleIncomingMessages() {
  if (!nm->getWS()->getListener()->isAccepting()) {
    LOG(WARNING) << "iocWsListener_ not accepting incoming messages";
  } else {
    // LOG(INFO) << "handleIncomingMessages";
    wsGameManager->processIncomingMessages();
  }
  wrtcGameManager->processIncomingMessages();
}

} // namespace gameserver

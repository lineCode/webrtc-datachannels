#include "GameClient.hpp" // IWYU pragma: associated

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

namespace gameclient {

std::shared_ptr<::gloer::net::NetworkManager> GameClient::nm = nullptr;

void GameClient::init(std::weak_ptr<GameClient> game) {
  wsGameManager = std::make_shared<WSServerManager>(game);
  wrtcGameManager = std::make_shared<WRTCServerManager>(game);
}

void GameClient::handleIncomingMessages() {
  // LOG(INFO) << "WS handleIncomingMessages";

  if (!nm->getWS()->getListener()->isAccepting()) {
    LOG(WARNING) << "iocWsListener_ not accepting incoming messages";
  } else {
    wsGameManager->processIncomingMessages();
  }

  // LOG(INFO) << "WRTC handleIncomingMessages";
  wrtcGameManager->processIncomingMessages();
}

} // namespace gameclient

/*
 * Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include "GameServer.hpp"
#include "WRTCServerManager.hpp"
#include "WSServerManager.hpp"
#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
#include "algo/StringUtils.hpp"
#include "algo/TickManager.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/wrtc/WRTCSession.hpp"
#include "net/ws/WsListener.hpp"
#include "net/ws/WsServer.hpp"
#include "net/ws/WsSession.hpp"
#include "storage/path.hpp"
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/system/error_code.hpp>
#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <enum.h>
#include <filesystem>
//#include <folly/Singleton.h>
//#include <folly/init/Init.h>
#include "GameServer.hpp"
#include "WRTCServerManager.hpp"
#include "WSServerManager.hpp"
#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
#include "algo/TickManager.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/wrtc/WRTCSession.hpp"
#include "net/ws/WsListener.hpp"
#include "net/ws/WsServer.hpp"
#include "net/ws/WsSession.hpp"
#include "storage/path.hpp"
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/system/error_code.hpp>
#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <enum.h>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <new>
#include <rapidjson/document.h>
#include <rapidjson/error/error.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
//#include <folly/Singleton.h>
//#include <folly/init/Init.h>
#include <functional>
#include <iostream>
#include <map>
#include <new>
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <rapidjson/error/error.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace fs = std::filesystem; // from <filesystem>
using namespace std::chrono_literals;

using namespace ::gloer::net::ws;
using namespace ::gloer::net::wrtc;
using namespace ::gloer::algo;

using namespace ::gameserver;

namespace {
// folly::Singleton<GameServer> gGame;
}

static void printNumOfCores() {
  unsigned int c = std::thread::hardware_concurrency();
  LOG(INFO) << "Number of cores: " << c;
  const size_t minCores = 4;
  if (c < minCores) {
    LOG(INFO) << "Too low number of cores! Prefer servers with at least " << minCores << " cores";
  }
}

static std::string createTestMessage(std::string_view str) {
  using namespace ::gloer::net::ws;
  using namespace ::gloer::net::wrtc;
  using namespace ::gloer::algo;

  rapidjson::Document message_object;
  message_object.SetObject();
  message_object.AddMember("type", rapidjson::StringRef(Opcodes::opcodeToStr(WS_OPCODE::PING)),
                           message_object.GetAllocator());
  rapidjson::Value test_message_value;
  test_message_value.SetString(rapidjson::GenericStringRef<char>(str.data(), str.size()));
  rapidjson::Value message_payload;
  message_payload.SetObject();
  message_payload.AddMember("testMessageKey", test_message_value, message_object.GetAllocator());
  message_object.AddMember("payload", message_payload, message_object.GetAllocator());
  rapidjson::StringBuffer strbuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  bool done = message_object.Accept(writer);
  if (!done) {
    LOG(WARNING) << "OnIceCandidate: INVALID JSON!";
    return "";
  }
  return strbuf.GetString();
}

int main(int argc, char* argv[]) {
  // folly::init(&argc, &argv, /* remove recognized gflags */ true);
  using namespace ::gloer::net::ws;
  using namespace ::gloer::net::wrtc;
  using namespace ::gloer::algo;

  std::locale::global(std::locale::classic()); // https://stackoverflow.com/a/18981514/10904212

  size_t WRTCTickFreq = 100; // 1/Freq
  size_t WRTCTickNum = 0;

  size_t WSTickFreq = 100; // 1/Freq
  size_t WSTickNum = 0;

  gloer::log::Logger lg; // inits Logger
  LOG(INFO) << "created Logger...";

  LOG(INFO) << "Starting client..";

  // std::weak_ptr<GameServer> gameInstance = folly::Singleton<GameServer>::try_get();
  // gameInstance->init(gameInstance);

  std::shared_ptr<GameServer> gameInstance = std::make_shared<GameServer>();
  gameInstance->init(gameInstance);

  printNumOfCores();

  uint32_t sendCounter = 0;

  const std::string clientNickname = gloer::algo::genGuid();

  const ::fs::path workdir = gloer::storage::getThisBinaryDirectoryPath();

  // TODO: support async file read, use futures or std::async
  // NOTE: future/promise Should Not Be Coupled to std::thread Execution Agents
  gloer::config::ServerConfig serverConfig(
      ::fs::path{workdir / gloer::config::ASSETS_DIR / gloer::config::CONFIGS_DIR /
                 gloer::config::CONFIG_NAME},
      workdir);

  // NOTE: Tell the socket to bind to port 0 - random port
  serverConfig.wsPort_ = static_cast<unsigned short>(0);

  LOG(INFO) << "make_shared NetworkManager...";
  gameInstance->nm = std::make_shared<::gloer::net::NetworkManager>(serverConfig);

  // TODO: print active sessions

  // TODO: destroy inactive wrtc sessions (by timer)

  LOG(INFO) << "runAsClient...";

  gameInstance->nm->runAsClient(serverConfig);

  LOG(INFO) << "Set getWS()->SetOnNewSessionHandler...";

  gameInstance->nm->getWS()->SetOnNewSessionHandler(
      [&gameInstance](std::shared_ptr<WsSession> sess) {
        sess->SetOnMessageHandler(std::bind(&WSServerManager::handleIncomingJSON,
                                            gameInstance->wsGameManager, std::placeholders::_1,
                                            std::placeholders::_2));
        sess->SetOnCloseHandler(std::bind(&WSServerManager::handleClose,
                                          gameInstance->wsGameManager, std::placeholders::_1));
      });

  LOG(INFO) << "Set getWRTC()->SetOnNewSessionHandler...";

  gameInstance->nm->getWRTC()->SetOnNewSessionHandler(
      [&gameInstance](std::shared_ptr<WRTCSession> sess) {
        sess->SetOnMessageHandler(std::bind(&WRTCServerManager::handleIncomingJSON,
                                            gameInstance->wrtcGameManager, std::placeholders::_1,
                                            std::placeholders::_2));
        sess->SetOnCloseHandler(std::bind(&WRTCServerManager::handleClose,
                                          gameInstance->wrtcGameManager, std::placeholders::_1));
      });

  LOG(INFO) << "Starting client loop for event queue";

  // processRecievedMsgs
  TickManager<std::chrono::milliseconds> tm(50ms);

  // Create the session and run it
  const auto newSessId = "@clientSideServerId@";
  auto newWsSession = gameInstance->nm->getWS()->getListener()->addClientSession(newSessId);
  if (!newWsSession || !newWsSession.get()) {
    LOG(WARNING) << "addClientSession failed ";
    gameInstance->nm->finishServers();
    LOG(WARNING) << "exiting...";
    return EXIT_SUCCESS;
  }

  const std::string hostToConnect = "localhost";
  const std::string portToConnect = "8080";
  newWsSession->connectAsClient(hostToConnect, portToConnect);

  // NOTE: need wait connected state
  LOG(WARNING) << "connecting to " << hostToConnect << ":" << portToConnect << "...";
  bool isConnected = newWsSession->waitForConnect(/* maxWait_ms */ 5000);
  if (!isConnected) {
    LOG(WARNING) << "waitForConnect: Can`t connect to " << hostToConnect << ":" << portToConnect;
    gameInstance->nm->finishServers();
    LOG(WARNING) << "exiting...";
    return EXIT_SUCCESS;
  }
  LOG(WARNING) << "newWsSession->runAsClient...";
  newWsSession->runAsClient();

  tm.addTickHandler(
      TickHandler("handleAllPlayerMessages", [&gameInstance, &newSessId, &newWsSession]() {
        // Handle queued incoming messages
        gameInstance->handleIncomingMessages();
      }));

  tm.addTickHandler(TickHandler(
      "sendTestMessage", [&gameInstance, &sendCounter, &newWsSession, &clientNickname]() {
        const std::string msg =
            createTestMessage(std::to_string(sendCounter++) + " message from " + clientNickname);
        newWsSession->send(msg);
      }));

  while (tm.needServerRun()) {
    tm.tick();
  }

  // TODO: sendProcessedMsgs in separate thread

  // (If we get here, it means we got a SIGINT or SIGTERM)
  LOG(WARNING) << "If we get here, it means we got a SIGINT or SIGTERM";

  gameInstance->nm->finishServers();

  // folly::SingletonVault::singleton()->destroyInstances();
  gameInstance.reset();

  lg.shutDownLogging();

  return EXIT_SUCCESS;
}

/*
 * Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

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
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <p2p/base/portallocator.h>
#include <rapidjson/encodings.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem; // from <filesystem>

using namespace std::chrono_literals;

static const char testMessageKey[] = "testMessage";

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
  message_payload.AddMember(testMessageKey, test_message_value, message_object.GetAllocator());
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
  using namespace ::gloer::net::ws;
  using namespace ::gloer::net::wrtc;
  using namespace ::gloer::algo;

  std::locale::global(std::locale::classic()); // https://stackoverflow.com/a/18981514/10904212

  gloer::log::Logger::instance(); // inits Logger

  LOG(INFO) << "Starting client..";

  printNumOfCores();

  uint32_t sendCounter = 0;

  const std::string clientNickname = gloer::algo::genGuid();

  const fs::path workdir = gloer::storage::getThisBinaryDirectoryPath();

  // TODO: support async file read, use futures or std::async
  // NOTE: future/promise Should Not Be Coupled to std::thread Execution Agents
  gloer::config::ServerConfig serverConfig(
      ::fs::path{workdir / gloer::config::ASSETS_DIR / gloer::config::CONFIGS_DIR /
                 gloer::config::CONFIG_NAME},
      workdir);

  // NOTE Tell the socket to bind to port 0 - random port
  serverConfig.wsPort_ = static_cast<unsigned short>(0);

  auto nm = std::make_shared<gloer::net::NetworkManager>(serverConfig);

  nm->runAsClient(serverConfig); // <<<<<

  // process recieved messages with some period
  TickManager<std::chrono::milliseconds> tm(50ms);

  // Create the session and run it
  const auto newSessId = "@clientSideServerId@";
  auto newWsSession = nm->getWS()->getListener()->addClientSession(newSessId);
  if (!newWsSession || !newWsSession.get()) {
    LOG(WARNING) << "addClientSession failed ";
    nm->finishServers();
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
    nm->finishServers();
    LOG(WARNING) << "exiting...";
    return EXIT_SUCCESS;
  }
  LOG(WARNING) << "newWsSession->runAsClient...";
  newWsSession->runAsClient();

  tm.addTickHandler(TickHandler("handleAllPlayerMessages", [&nm, &newSessId, &newWsSession]() {
    // Handle queued incoming messages
    nm->handleIncomingMessages();
  }));

  tm.addTickHandler(
      TickHandler("sendTestMessage", [&nm, &sendCounter, &newWsSession, &clientNickname]() {
        const std::string msg =
            createTestMessage(std::to_string(sendCounter++) + " message from " + clientNickname);
        newWsSession->send(std::make_shared<std::string>(msg));
      }));

  while (tm.needServerRun()) {
    tm.tick();
  }

  // (If we get here, it means we got a SIGINT or SIGTERM)
  LOG(WARNING) << "If we get here, it means we got a SIGINT or SIGTERM";

  nm->finishServers();

  return EXIT_SUCCESS;
}

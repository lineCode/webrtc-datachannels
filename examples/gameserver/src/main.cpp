/*
 * Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

/** @file
 * @brief Game server start point
 */

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
//#include <folly/Singleton.h>
//#include <folly/init/Init.h>
#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/wrtc/WRTCSession.hpp"
#include "net/wrtc/wrtc.hpp"
#include "net/ws/WsListener.hpp"
#include "net/ws/WsSession.hpp"
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <cstddef>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <net/core.hpp>
#include <new>
#include <rapidjson/document.h>
#include <rapidjson/error/error.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <webrtc/rtc_base/bind.h>
#include <webrtc/rtc_base/checks.h>
#include <webrtc/rtc_base/rtccertificategenerator.h>
#include <webrtc/rtc_base/ssladapter.h>

namespace {

using namespace ::gloer::net;
using namespace ::gloer::net::wrtc;
using namespace ::gloer::net::ws;

static void pingCallback(std::shared_ptr<WsSession> clientSession, NetworkManager* nm,
                         std::shared_ptr<std::string> messageBuffer) {
  if (!messageBuffer || !messageBuffer.get() || messageBuffer->empty()) {
    LOG(WARNING) << "WsServer: Invalid messageBuffer";
    return;
  }

  if (!clientSession || !clientSession.get()) {
    LOG(WARNING) << "WSServer invalid clientSession!";
    return;
  }

  if (!nm) {
    LOG(WARNING) << "WSServer invalid NetworkManager!";
    return;
  }

  std::string dataCopy = *messageBuffer.get();

  // const std::string incomingStr = beast::buffers_to_string(messageBuffer->data());
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "pingCallback incomingMsg=" << messageBuffer->c_str();

  // send same message back (ping-pong)
  if (clientSession && clientSession.get() && clientSession->isOpen() &&
      !clientSession->isExpired())
    clientSession->send(dataCopy);
}

static void candidateCallback(std::shared_ptr<WsSession> clientSession, NetworkManager* nm,
                              std::shared_ptr<std::string> messageBuffer) {
  // const std::string incomingStr = beast::buffers_to_string(messageBuffer->data());

  if (!messageBuffer || !messageBuffer.get() || messageBuffer->empty()) {
    LOG(WARNING) << "WsServer: Invalid messageBuffer";
    return;
  }

  if (!clientSession || !clientSession.get()) {
    LOG(WARNING) << "WSServer invalid clientSession!";
    return;
  }

  if (!nm) {
    LOG(WARNING) << "WSServer invalid NetworkManager!";
    return;
  }

  std::string dataCopy = *messageBuffer.get();

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "candidateCallback incomingMsg=" << dataCopy;

  // todo: pass parsed
  rapidjson::Document message_object;
  message_object.Parse(dataCopy.c_str());

  // Server receives Client’s ICE candidates, then finds its own ICE
  // candidates & sends them to Client
  LOG(INFO) << "type == candidate";

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "m_WS->WSQueue.dispatch type == candidate";
  rapidjson::Document message_object1;     // TODO
  message_object1.Parse(dataCopy.c_str()); // TODO

  auto spt =
      clientSession->getWRTCSession().lock(); // Has to be copied into a shared_ptr before usage

  /*auto handle = OnceFunctor([clientSession, spt, nm, &message_object1]() {
    if (spt) {
      spt->createAndAddIceCandidate(message_object1);
    } else {
      LOG(WARNING) << "wrtcSess_ expired";
      return;
    }
  });

  nm->getWRTC()->workerThread_->Post(RTC_FROM_HERE, handle);*/

  if (spt) {
    spt->createAndAddIceCandidateFromJson(message_object1);
  } else {
    LOG(WARNING) << "wrtcSess_ expired";
    return;
  }
}

// client send offer to server
static void offerCallback(std::shared_ptr<WsSession> clientSession, NetworkManager* nm,
                          std::shared_ptr<std::string> messageBuffer) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WS: type == offer";

  if (!messageBuffer || !messageBuffer.get() || messageBuffer->empty()) {
    LOG(WARNING) << "WsServer: Invalid messageBuffer";
    return;
  }

  if (!clientSession || !clientSession.get()) {
    LOG(WARNING) << "WSServer invalid clientSession!";
    return;
  }

  if (!nm) {
    LOG(WARNING) << "WSServer invalid NetworkManager!";
    return;
  }

  std::string dataCopy = *messageBuffer.get();

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "offerCallback incomingMsg=" << dataCopy.c_str();

  // todo: pass parsed
  rapidjson::Document message_object;
  message_object.Parse(dataCopy.c_str());

  // TODO: don`t create datachennel for same client twice?
  LOG(INFO) << "type == offer";

  rapidjson::Document message_obj;     // TODO
  message_obj.Parse(dataCopy.c_str()); // TODO

  const auto sdp = WRTCServer::sessionDescriptionStrFromJson(message_obj);

  /*auto handle = OnceFunctor([clientSession, nm, sdp]() {
    WRTCServer::setRemoteDescriptionAndCreateAnswer(clientSession, nm, sdp);
  });
  nm->getWRTC()->signaling_thread()->Post(RTC_FROM_HERE, handle);*/

  WRTCServer::setRemoteDescriptionAndCreateAnswer(clientSession, nm, sdp);

  LOG(INFO) << "WS: added type == offer";
}

static void answerCallback(std::shared_ptr<WsSession> clientSession, NetworkManager* nm,
                           std::shared_ptr<std::string> messageBuffer) {
  LOG(WARNING) << "no answerCallback on server";
}

} // namespace

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

int main(int argc, char* argv[]) {
  // folly::init(&argc, &argv, /* remove recognized gflags */ true);
  using namespace ::gloer::net::ws;
  using namespace ::gloer::net::wrtc;
  using namespace ::gloer::algo;

  std::locale::global(std::locale::classic()); // https://stackoverflow.com/a/18981514/10904212

  size_t WRTCTickFreq = 200; // 1/Freq
  size_t WRTCTickNum = 0;

  size_t WSTickFreq = 200; // 1/Freq
  size_t WSTickNum = 0;

  gloer::log::Logger lg; // inits Logger
  LOG(INFO) << "created Logger...";

  // std::weak_ptr<GameServer> gameInstance = folly::Singleton<GameServer>::try_get();
  // gameInstance->init(gameInstance);

  std::shared_ptr<GameServer> gameInstance = std::make_shared<GameServer>();
  gameInstance->init(gameInstance);

  printNumOfCores();

  const ::fs::path workdir = gloer::storage::getThisBinaryDirectoryPath();

  // TODO: support async file read, use futures or std::async
  // NOTE: future/promise Should Not Be Coupled to std::thread Execution Agents
  const gloer::config::ServerConfig serverConfig(
      ::fs::path{workdir / gloer::config::ASSETS_DIR / gloer::config::CONFIGS_DIR /
                 gloer::config::CONFIG_NAME},
      workdir);

  LOG(INFO) << "make_shared NetworkManager...";
  gameInstance->nm = std::make_shared<::gloer::net::NetworkManager>(serverConfig);

  // TODO: print active sessions

  // TODO: destroy inactive wrtc sessions (by timer)

  LOG(INFO) << "runAsServer...";

  gameInstance->nm->runAsServer(serverConfig);

  using namespace gloer;
  const WsNetworkOperation PING_OPERATION =
      WsNetworkOperation(algo::WS_OPCODE::PING, algo::Opcodes::opcodeToStr(algo::WS_OPCODE::PING));
  gameInstance->nm->getWS()->addCallback(PING_OPERATION, &pingCallback);

  const WsNetworkOperation CANDIDATE_OPERATION = WsNetworkOperation(
      algo::WS_OPCODE::CANDIDATE, algo::Opcodes::opcodeToStr(algo::WS_OPCODE::CANDIDATE));
  gameInstance->nm->getWS()->addCallback(CANDIDATE_OPERATION, &candidateCallback);

  const WsNetworkOperation OFFER_OPERATION = WsNetworkOperation(
      algo::WS_OPCODE::OFFER, algo::Opcodes::opcodeToStr(algo::WS_OPCODE::OFFER));
  gameInstance->nm->getWS()->addCallback(OFFER_OPERATION, &offerCallback);

  const WsNetworkOperation ANSWER_OPERATION = WsNetworkOperation(
      algo::WS_OPCODE::ANSWER, algo::Opcodes::opcodeToStr(algo::WS_OPCODE::ANSWER));
  gameInstance->nm->getWS()->addCallback(ANSWER_OPERATION, &answerCallback);

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

  LOG(INFO) << "Starting server loop for event queue";

  // processRecievedMsgs
  TickManager<std::chrono::milliseconds> tm(50ms);

  tm.addTickHandler(TickHandler("handleAllPlayerMessages", [&gameInstance]() {
    // TODO: merge responses for same Player (NOTE: packet size limited!)

    // TODO: move game logic to separete thread or service

    // Handle queued incoming messages
    gameInstance->handleIncomingMessages();
  }));

  {
    tm.addTickHandler(TickHandler("WSTick", [&gameInstance, &WSTickFreq, &WSTickNum]() {
      WSTickNum++;
      if (WSTickNum < WSTickFreq) {
        return;
      } else {
        WSTickNum = 0;
      }
      // LOG(WARNING) << "WSTick! " << gameInstance->nm->getWS()->getSessionsCount();
      // send test data to all players
      std::chrono::system_clock::time_point nowTp = std::chrono::system_clock::now();
      std::time_t t = std::chrono::system_clock::to_time_t(nowTp);
      std::string msg = "WS server_time: ";
      msg += std::ctime(&t);
      msg += ";Total WS connections:";
      msg += std::to_string(gameInstance->nm->getWS()->getSessionsCount());
      const std::unordered_map<std::string, std::shared_ptr<WsSession>>& sessions =
          gameInstance->nm->getWS()->getSessions();
      msg += ";SESSIONS:[";
      for (auto& it : sessions) {
        std::shared_ptr<WsSession> wss = it.second;
        msg += it.first;
        msg += "=";
        if (!wss || !wss.get()) {
          msg += "EMPTY";
        } else {
          msg += wss->getId();
        }
      }
      msg += "]SESSIONS";

      gameInstance->nm->getWS()->sendToAll(msg);
      gameInstance->nm->getWS()->doToAllSessions(
          [&](const std::string& sessId, std::shared_ptr<WsSession> session) {
            if (!session || !session.get()) {
              LOG(WARNING) << "WSTick: Invalid WsSession ";
              return;
            }

            auto wsSessId = session->getId(); // remember id before session deletion

            session->send("Your WS id: " + wsSessId);
          });
    }));
  }

  {
    tm.addTickHandler(TickHandler("WRTCTick", [&gameInstance, &WRTCTickFreq, &WRTCTickNum]() {
      WRTCTickNum++;
      if (WRTCTickNum < WRTCTickFreq) {
        return;
      } else {
        WRTCTickNum = 0;
      }
      // LOG(WARNING) << "WRTCTick! " << gameInstance->nm->getWRTC()->getSessionsCount();
      // send test data to all players
      std::chrono::system_clock::time_point nowTp = std::chrono::system_clock::now();
      std::time_t t = std::chrono::system_clock::to_time_t(nowTp);
      std::string msg = "WRTC server_time: ";
      msg += std::ctime(&t);
      msg += ";Total WRTC connections:";
      msg += std::to_string(gameInstance->nm->getWRTC()->getSessionsCount());
      const std::unordered_map<std::string, std::shared_ptr<WRTCSession>>& sessions =
          gameInstance->nm->getWRTC()->getSessions();
      msg += ";SESSIONS:[";
      for (auto& it : sessions) {
        std::shared_ptr<WRTCSession> wrtcs = it.second;
        msg += it.first;
        msg += "=";
        if (!wrtcs || !wrtcs.get()) {
          msg += "EMPTY";
        } else {
          msg += wrtcs->getId();
        }
      }
      msg += "]SESSIONS";

      gameInstance->nm->getWRTC()->sendToAll(msg);
      gameInstance->nm->getWRTC()->doToAllSessions(
          [&](const std::string& sessId, std::shared_ptr<WRTCSession> session) {
            if (!session || !session.get()) {
              LOG(WARNING) << "WRTCTick: Invalid WRTCSession ";
              return;
            }

            auto wrtcSessId = session->getId(); // remember id before session deletion

            session->send("Your WRTC id: " + wrtcSessId);
          });
    }));
  }

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
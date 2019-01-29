/*
 * Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include "algo/TickManager.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/wrtc/WRTCSession.hpp"
#include "net/ws/WsServer.hpp"
#include "net/ws/WsSession.hpp"
#include "storage/path.hpp"
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem; // from <filesystem>

using namespace std::chrono_literals;

int main(int argc, char* argv[]) {
  using namespace ::gloer::net::ws;
  using namespace ::gloer::net::wrtc;
  using namespace ::gloer::algo;

  size_t WRTCTickFreq = 100; // 1/Freq
  size_t WRTCTickNum = 0;

  size_t WSTickFreq = 100; // 1/Freq
  size_t WSTickNum = 0;

  gloer::log::Logger::instance(); // inits Logger

  {
    unsigned int c = std::thread::hardware_concurrency();
    LOG(INFO) << "Number of cores: " << c;
    const size_t minCores = 4;
    if (c < minCores) {
      LOG(INFO) << "Too low number of cores! Prefer servers with at least " << minCores << " cores";
    }
  }

  const ::fs::path workdir = gloer::storage::getThisBinaryDirectoryPath();

  // TODO: support async file read, use futures or std::async
  // NOTE: future/promise Should Not Be Coupled to std::thread Execution Agents
  const gloer::config::ServerConfig serverConfig(
      ::fs::path{workdir / gloer::config::ASSETS_DIR / gloer::config::CONFIGS_DIR /
                 gloer::config::CONFIG_NAME},
      workdir);

  auto nm = std::make_shared<::gloer::net::NetworkManager>(serverConfig);

  // TODO: print active sessions

  // TODO: destroy inactive wrtc sessions (by timer)

  nm->run(serverConfig);

  LOG(INFO) << "Starting server loop for event queue";

  // processRecievedMsgs
  TickManager<std::chrono::milliseconds> tm(50ms);

  tm.addTickHandler(TickHandler("handleAllPlayerMessages", [&nm]() {
    // TODO: merge responses for same Player (NOTE: packet size limited!)

    // TODO: move game logic to separete thread or service

    // Handle queued incoming messages
    nm->handleIncomingMessages();
  }));

  {
    tm.addTickHandler(TickHandler("WSTick", [&nm, &WSTickFreq, &WSTickNum]() {
      WSTickNum++;
      if (WSTickNum < WSTickFreq) {
        return;
      } else {
        WSTickNum = 0;
      }
      LOG(WARNING) << "WSTick! " << nm->getWS()->getSessionsCount();
      // send test data to all players
      std::chrono::system_clock::time_point nowTp = std::chrono::system_clock::now();
      std::time_t t = std::chrono::system_clock::to_time_t(nowTp);
      std::string msg = "WS server_time: ";
      msg += std::ctime(&t);
      msg += ";Total WS connections:";
      msg += std::to_string(nm->getWS()->getSessionsCount());
      const std::unordered_map<std::string, std::shared_ptr<WsSession>>& sessions =
          nm->getWS()->getSessions();
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

      nm->getWS()->sendToAll(msg);
      nm->getWS()->doToAllSessions(
          [&](const std::string& sessId, std::shared_ptr<WsSession> session) {
            if (!session || !session.get()) {
              LOG(WARNING) << "WSTick: Invalid WsSession ";
              return;
            }
            session->send("Your WS id: " + session->getId());
          });
    }));
  }

  {
    tm.addTickHandler(TickHandler("WRTCTick", [&nm, &WRTCTickFreq, &WRTCTickNum]() {
      WRTCTickNum++;
      if (WRTCTickNum < WRTCTickFreq) {
        return;
      } else {
        WRTCTickNum = 0;
      }
      LOG(WARNING) << "WRTCTick! " << nm->getWRTC()->getSessionsCount();
      // send test data to all players
      std::chrono::system_clock::time_point nowTp = std::chrono::system_clock::now();
      std::time_t t = std::chrono::system_clock::to_time_t(nowTp);
      std::string msg = "WRTC server_time: ";
      msg += std::ctime(&t);
      msg += ";Total WRTC connections:";
      msg += std::to_string(nm->getWRTC()->getSessionsCount());
      const std::unordered_map<std::string, std::shared_ptr<WRTCSession>>& sessions =
          nm->getWRTC()->getSessions();
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

      nm->getWRTC()->sendToAll(msg);
      nm->getWRTC()->doToAllSessions(
          [&](const std::string& sessId, std::shared_ptr<WRTCSession> session) {
            if (!session || !session.get()) {
              LOG(WARNING) << "WRTCTick: Invalid WRTCSession ";
              return;
            }
            session->send("Your WRTC id: " + session->getId());
          });
    }));
  }

  while (tm.needServerRun()) {
    tm.tick();
  }

  // TODO: sendProcessedMsgs in separate thread

  // (If we get here, it means we got a SIGINT or SIGTERM)
  LOG(WARNING) << "If we get here, it means we got a SIGINT or SIGTERM";

  nm->finish();

  return EXIT_SUCCESS;
}

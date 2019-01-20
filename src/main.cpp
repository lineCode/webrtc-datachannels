#include "config/ServerConfig.hpp"
#include "filesystem/path.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include "net/webrtc/WRTCServer.hpp"
#include "net/webrtc/WRTCSession.hpp"
#include "net/websockets/WsServer.hpp"
#include "net/websockets/WsSession.hpp"
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

class TickHandler {
public:
  TickHandler(const std::string& id, std::function<void()> fn) : id_(id), fn_(fn) {}
  const std::string id_;
  const std::function<void()> fn_;
};

template <typename PeriodType> class TickManager {
public:
  TickManager(const PeriodType& serverNetworkUpdatePeriod)
      : serverNetworkUpdatePeriod_(serverNetworkUpdatePeriod) {}

  void tick() {
    std::this_thread::sleep_for(serverNetworkUpdatePeriod_);
    for (const TickHandler& it : tickHandlers_) {
      // LOG(INFO) << "tick() for " << it.id_;
      it.fn_();
    }
  }

  bool needServerRun() const { return needServerRun_; }

  void stop() { needServerRun_ = false; }

  void addTickHandler(const TickHandler& tickHandler) { tickHandlers_.push_back(tickHandler); }

  void getTickHandlers() const { return tickHandlers_; }

private:
  PeriodType serverNetworkUpdatePeriod_;

  std::vector<TickHandler> tickHandlers_;

  bool needServerRun_ = true;
};

int main(int argc, char* argv[]) {

  size_t WRTCTickFreq = 20; // 1/Freq
  size_t WRTCTickNum = 0;

  size_t WSTickFreq = 20; // 1/Freq
  size_t WSTickNum = 0;

  utils::log::Logger::instance(); // inits Logger

  const fs::path workdir = utils::filesystem::getThisBinaryDirectoryPath();

  const utils::config::ServerConfig serverConfig(
      fs::path{workdir / utils::config::ASSETS_DIR / utils::config::CONFIG_NAME}, workdir);

  auto nm = std::make_shared<utils::net::NetworkManager>(serverConfig);

  // TODO: print active sessions

  // TODO: destroy inactive wrtc sessions (by timer)

  nm->run(serverConfig);

  LOG(INFO) << "Starting server loop for event queue";

  TickManager<std::chrono::milliseconds> tm(50ms);

  tm.addTickHandler(TickHandler("handleAllPlayerMessages", [&nm]() {
    // TODO: merge responses for same Player (NOTE: packet size limited!)

    // TODO: move game logic to separete thread or service

    // Handle queued incoming messages
    nm->handleAllPlayerMessages();
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
      nm->getWS()->sendToAll(msg);
      nm->getWS()->doToAllSessions([&](std::shared_ptr<utils::net::WsSession> session) {
        if (!session) {
          LOG(WARNING) << "WSTick: Invalid WsSession ";
          return;
        }
        session.get()->send("Your WS id: " + session.get()->getId());
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
      nm->getWRTC()->sendToAll(msg);
      nm->getWRTC()->doToAllSessions([&](std::shared_ptr<utils::net::WRTCSession> session) {
        if (!session) {
          LOG(WARNING) << "WRTCTick: Invalid WRTCSession ";
          return;
        }
        session.get()->send("Your WRTC id: " + session.get()->getId());
      });
    }));
  }

  while (tm.needServerRun()) {
    tm.tick();
  }

  // (If we get here, it means we got a SIGINT or SIGTERM)
  LOG(WARNING) << "If we get here, it means we got a SIGINT or SIGTERM";

  nm->finish();

  return EXIT_SUCCESS;
}

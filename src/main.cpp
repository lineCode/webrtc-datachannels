#include "config/ServerConfig.hpp"
#include "filesystem/path.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
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

typedef std::function<void()> TickHandler;

template <typename PeriodType> class TickManager {
public:
  TickManager(const PeriodType& serverNetworkUpdatePeriod)
      : serverNetworkUpdatePeriod_(serverNetworkUpdatePeriod) {}

  void tick() {
    std::this_thread::sleep_for(serverNetworkUpdatePeriod_);
    for (const auto& it : tickHandlers_) {
      it();
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

  utils::log::Logger::instance(); // inits Logger

  const fs::path workdir = utils::filesystem::getThisBinaryDirectoryPath();

  const utils::config::ServerConfig serverConfig(
      fs::path{workdir / utils::config::ASSETS_DIR / utils::config::CONFIG_NAME}, workdir);

  auto nm = std::make_shared<utils::net::NetworkManager>(serverConfig);

  // TODO: print active sessions

  // TODO: destroy inactive wrtc sessions (by timer)

  nm->run(serverConfig);

  LOG(INFO) << "Starting server loop for event queue";

  TickManager<std::chrono::milliseconds> tm(1ms);

  tm.addTickHandler([&nm]() {
    // TODO: merge responses for same Player (NOTE: packet size limited!)

    // TODO: move game logic to separete thread or service

    // Handle queued incoming messages
    nm->handleAllPlayerMessages();
  });

  /*tm.addTickHandler([&nm]() {
    // send test data to all players
    std::time_t t = std::chrono::system_clock::to_time_t(nowTp);
      std::string msg = "server_time: ";
      msg += std::ctime(&t);
      sm->sendToAll(msg);
      sm->doToAllPlayers([&](std::shared_ptr<utils::net::WsSession> session) {
        session.get()->send("Your id: " + session.get()->getId());
      });
  });*/

  while (tm.needServerRun()) {
    tm.tick();
  }

  // (If we get here, it means we got a SIGINT or SIGTERM)
  LOG(WARNING) << "If we get here, it means we got a SIGINT or SIGTERM";

  nm->finish();

  return EXIT_SUCCESS;
}

#include "config/ServerConfig.hpp"
#include "filesystem/path.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace fs = std::filesystem; // from <filesystem>

bool doServerRun = true;

int main(int argc, char* argv[]) {
  using namespace std::chrono_literals;

  utils::log::Logger::instance(); // inits Logger

  const fs::path workdir = utils::filesystem::getThisBinaryDirectoryPath();

  const utils::config::ServerConfig serverConfig(
      fs::path{workdir / utils::config::ASSETS_DIR /
               utils::config::CONFIG_NAME},
      workdir);

  auto nm = std::make_shared<utils::net::NetworkManager>(serverConfig);

  nm->runIocWsListener(serverConfig);

  nm->runWsThreads(serverConfig);

  nm->runWrtcThreads(serverConfig);

  LOG(INFO) << "Starting server loop for event queue";

  auto serverNetworkUpdatePeriod = 50ms;
  while (doServerRun) {
    // TODO: event queue
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(serverNetworkUpdatePeriod);
    /*std::chrono::system_clock::time_point nowTp =
        std::chrono::system_clock::now();*/

    // Handle queued incoming messages
    nm->handleAllPlayerMessages();

    // send test data to all players
    /*std::time_t t = std::chrono::system_clock::to_time_t(nowTp);
    std::string msg = "server_time: ";
    msg += std::ctime(&t);
    sm->sendToAll(msg);
    sm->doToAllPlayers([&](std::shared_ptr<utils::net::WsSession> session) {
      session.get()->send("Your id: " + session.get()->getId());
    });*/
  } /*
       [sp = shared_from_this()](
           beast::error_code ec, std::size_t bytes)
       {
     sp->on_write(ec, bytes);
       });*/
  // (If we get here, it means we got a SIGINT or SIGTERM)
  LOG(WARNING) << "If we get here, it means we got a SIGINT or SIGTERM";

  nm->finishWsThreads();

  return EXIT_SUCCESS;
}

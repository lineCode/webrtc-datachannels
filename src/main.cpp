#include "config/ServerConfig.hpp"
#include "filesystem/path.hpp"
#include "log/Logger.hpp"
#include "lua/LuaScript.hpp"
#include "net/NetworkManager.hpp"
#include "net/WsListener.hpp"
#include "net/WsSessionManager.hpp"
#include <boost/asio/basic_streambuf.hpp> // IWYU pragma: keep
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/websocket/error.hpp>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace boost {
namespace system {
class error_code;
} // namespace system
} // namespace boost

namespace sol {
class state;
} // namespace sol

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

bool doServerRun = true;

// Run the I/O service on the requested number of threads
std::vector<std::thread> wsThreads;

/// Block until SIGINT or SIGTERM is received.
void sigWait(net::io_context& ioc) {
  // Capture SIGINT and SIGTERM to perform a clean shutdown
  boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
  signals.async_wait([&](boost::system::error_code const&, int) {
    // Stop the `io_context`. This will cause `run()`
    // to return immediately, eventually destroying the
    // `io_context` and all of the sockets in it.
    std::cerr << "Called ioc.stop() on SIGINT or SIGTERM\n";
    ioc.stop();
    doServerRun = false;
  });
}

void runWsThreads(net::io_context& ioc,
                  const utils::config::ServerConfig& serverConfig) {
  wsThreads.reserve(serverConfig.threads);
  for (auto i = serverConfig.threads; i > 0; --i) {
    wsThreads.emplace_back([&ioc] { ioc.run(); });
  }
}

void finishWsThreads() {
  // Block until all the threads exit
  for (auto& t : wsThreads) {
    if (t.joinable()) {
      t.join();
    }
  }
}

int main(int argc, char* argv[]) {
  using namespace std::chrono_literals;

  utils::log::Logger::instance(); // inits Logger

  LOG(WARNING) << "This log call, may or may not happend before"
               << "the sinkHandle->call below";

  auto nm = std::make_shared<utils::net::NetworkManager>();

  const fs::path workdir = utils::filesystem::getThisBinaryDirectoryPath();
  utils::lua::LuaScript luaScript;
  sol::state* configScript = luaScript.loadScriptFile(fs::path{
      workdir / utils::config::ASSETS_DIR / utils::config::CONFIG_NAME});

  const utils::config::ServerConfig serverConfig(configScript);

  // The io_context is required for all I/O
  net::io_context ioc{serverConfig.threads};

  // Create and launch a listening port
  auto iocWsListener = std::make_shared<utils::net::WsListener>(
      ioc, tcp::endpoint{serverConfig.address, serverConfig.port},
      std::make_shared<std::string>(workdir.string()), nm);

  iocWsListener->run();

  // TODO sigWait(ioc);
  // TODO ioc.run();
  runWsThreads(ioc, serverConfig);

  std::cout << "Starting server loop for event queue\n";

  auto serverNetworkUpdatePeriod = 50ms;
  while (doServerRun) {
    // TODO: event queue
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(serverNetworkUpdatePeriod);
    std::chrono::system_clock::time_point p = std::chrono::system_clock::now();

    // Handle queued incoming messages
    nm->getWsSessionManager()->handleAllPlayerMessages();

    // send test data to all players
    /*std::time_t t = std::chrono::system_clock::to_time_t(p);
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
  std::cerr << "If we get here, it means we got a SIGINT or SIGTERM\n";

  finishWsThreads();

  return EXIT_SUCCESS;
}

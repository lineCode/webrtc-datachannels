#include "config/config.hpp"
#include "filesystem/path.hpp"
#include "lua/lua.hpp"
#include "net/SessionManager.hpp"
#include "net/WsSession.hpp"
#include <algorithm>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/config.hpp>
#include <boost/make_unique.hpp>
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

// Report a failure
static void on_listener_fail(beast::error_code ec, char const* what) {
  std::cerr << what << ": " << ec.message() << "\n";
}

// from 1 to inf
static uint32_t nextSessionId(std::shared_ptr<utils::net::SessionManager>& sm) {
  return sm->sessionsCount() + 1;
}

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener> {
  tcp::acceptor acceptor_;
  tcp::socket socket_;
  std::shared_ptr<std::string const> doc_root_;
  std::shared_ptr<utils::net::SessionManager> sm_;

public:
  listener(net::io_context& ioc, tcp::endpoint endpoint,
           std::shared_ptr<std::string const> const& doc_root,
           std::shared_ptr<utils::net::SessionManager> sm)
      : acceptor_(ioc), socket_(ioc), doc_root_(doc_root), sm_(sm) {
    beast::error_code ec;

    // Open the acceptor
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
      on_listener_fail(ec, "open");
      return;
    }

    // Allow address reuse
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
      on_listener_fail(ec, "set_option");
      return;
    }

    // Bind to the server address
    acceptor_.bind(endpoint, ec);
    if (ec) {
      on_listener_fail(ec, "bind");
      return;
    }

    // Start listening for connections
    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
      on_listener_fail(ec, "listen");
      return;
    }
  }

  // Start accepting incoming connections
  void run() {
    std::cout << "WS run\n";
    if (!isAccepting())
      return;
    do_accept();
  }

  void do_accept() {
    std::cout << "WS do_accept\n";
    acceptor_.async_accept(socket_,
                           std::bind(&listener::on_accept, shared_from_this(),
                                     std::placeholders::_1));
  }

  /**
   * @brief checks whether server is accepting new connections
   */
  bool isAccepting() const { return acceptor_.is_open(); }

  /**
   * @brief handles new connections and starts sessions
   */
  void on_accept(beast::error_code ec) {
    std::cout << "WS on_accept\n";
    if (ec) {
      on_listener_fail(ec, "accept");
    } else {
      // Create the session and run it
      auto newWsSession = std::make_shared<utils::net::WsSession>(
          std::move(socket_), sm_, nextSessionId(sm_));
      sm_->registerSession(newWsSession);
      newWsSession->run();
    }

    // Accept another connection
    do_accept();
  }
};

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

const std::string ASSETS_DIR = "assets";
const std::string CONFIG_NAME = "conf.lua";

int main(int argc, char* argv[]) {
  auto sm = std::make_shared<utils::net::SessionManager>();

  const fs::path workdir = utils::filesystem::getThisBinaryDirectoryPath();
  utils::lua::LuaScript luaScript;
  sol::state* configScript =
      luaScript.loadScriptFile(fs::path{workdir / ASSETS_DIR / CONFIG_NAME});

  const utils::config::ServerConfig serverConfig(configScript);

  // The io_context is required for all I/O
  net::io_context ioc{serverConfig.threads};

  // Create and launch a listening port
  auto iocListener = std::make_shared<listener>(
      ioc, tcp::endpoint{serverConfig.address, serverConfig.port},
      std::make_shared<std::string>(workdir.string()), sm);

  iocListener->run();

  // TODO sigWait(ioc);
  // TODO ioc.run();
  runWsThreads(ioc, serverConfig);

  std::cout << "Starting server loop for event queue\n";

  while (doServerRun) {
    // TODO: event queue
  }

  // (If we get here, it means we got a SIGINT or SIGTERM)
  std::cerr << "If we get here, it means we got a SIGINT or SIGTERM\n";

  finishWsThreads();

  std::cout << "EXIT_SUCCESS" << std::endl;
  return EXIT_SUCCESS;
}

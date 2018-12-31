#pragma once

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
#include <unordered_map>
#include <utility>
#include <vector>

namespace utils {
namespace net {

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class SessionManager;

// Accepts incoming connections and launches the sessions
class WsListener : public std::enable_shared_from_this<WsListener> {
  tcp::acceptor acceptor_;
  tcp::socket socket_;
  std::shared_ptr<std::string const> doc_root_;
  std::shared_ptr<utils::net::SessionManager> sm_;
  tcp::endpoint endpoint_;

public:
  WsListener(net::io_context& ioc, tcp::endpoint endpoint,
             std::shared_ptr<std::string const> const& doc_root,
             std::shared_ptr<utils::net::SessionManager> sm)
      : acceptor_(ioc), socket_(ioc), doc_root_(doc_root), sm_(sm),
        endpoint_(endpoint) {
    configureAcceptor();
  }

  void configureAcceptor();

  // Report a failure
  void on_WsListener_fail(beast::error_code ec, char const* what);

  // Start accepting incoming connections
  void run();

  void do_accept();

  /**
   * @brief checks whether server is accepting new connections
   */
  bool isAccepting() const { return acceptor_.is_open(); }

  /**
   * @brief handles new connections and starts sessions
   */
  void on_accept(beast::error_code ec);
};

} // namespace net
} // namespace utils

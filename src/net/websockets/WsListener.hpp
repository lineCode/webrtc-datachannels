#pragma once

#include <algorithm>
#include <boost/asio.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/_experimental/core/ssl_stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/config.hpp>
#include <boost/make_unique.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace gloer {
namespace net {

class NetworkManager;

/**
 * Accepts incoming connections and launches the sessions
 **/
class WsListener : public std::enable_shared_from_this<WsListener> {

public:
  WsListener(boost::asio::io_context& ioc, const boost::asio::ip::tcp::endpoint& endpoint,
             std::shared_ptr<std::string const> doc_root, NetworkManager* nm);

  void configureAcceptor();

  // Report a failure
  void on_WsListener_fail(boost::beast::error_code ec, char const* what);

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
  void on_accept(boost::beast::error_code ec);

private:
  boost::asio::ip::tcp::acceptor acceptor_;
  boost::asio::ip::tcp::socket socket_;
  std::shared_ptr<std::string const> doc_root_;
  NetworkManager* nm_;
  boost::asio::ip::tcp::endpoint endpoint_;
};

} // namespace net
} // namespace gloer

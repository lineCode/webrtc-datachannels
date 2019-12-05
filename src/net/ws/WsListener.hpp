#pragma once

/**
 * Accepts incoming connections and launches the sessions
 **/

#include <algorithm>
#include <boost/asio.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
//#include <boost/beast/experimental/core/ssl_stream.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/config.hpp>
#include <boost/make_unique.hpp>
#include <boost/beast/ssl.hpp>
#include <cstdlib>
#include <enum.h>
#include <functional>
#include <iostream>
#include <memory>
#include <net/core.hpp>
#include <net/NetworkManagerBase.hpp>
#include <string>
#include <thread>
#include <vector>

namespace gloer {
namespace net {

//class NetworkManager;

//class SessionBase;

namespace ws {

class WsSession;

//BETTER_ENUM(WS_LISTEN_MODE, uint32_t, CLIENT, SERVER, BOTH)

/**
 * Accepts incoming connections and launches the sessions
 **/
class WsListener : public std::enable_shared_from_this<WsListener> {

public:
  WsListener(boost::asio::io_context& ioc,
             ::boost::asio::ssl::context& ctx,
             const boost::asio::ip::tcp::endpoint& endpoint,
             std::shared_ptr<std::string const> doc_root, net::WSServerNetworkManager* nm);

  void configureAcceptor();

  // Report a failure
  void on_WsListener_fail(boost::beast::error_code ec, char const* what);

  // Start accepting incoming connections
  void run(/*WS_LISTEN_MODE mode*/);

  void do_accept();

  //std::shared_ptr<WsSession> addClientSession(const std::string& newSessId);

  /**
   * @brief checks whether server is accepting new connections
   */
  bool isAccepting() const { return acceptor_.is_open(); }

  /**
   * @brief handles new connections and starts sessions
   */
  void on_accept(boost::beast::error_code ec, ::boost::asio::ip::tcp::socket socket);

  void stop();

  //void setMode(WS_LISTEN_MODE mode);

private:
  boost::asio::ip::tcp::acceptor acceptor_;

  //boost::asio::ip::tcp::socket socket_;

  boost::asio::io_context& ioc_;

  std::shared_ptr<std::string const> doc_root_;

  net::WSServerNetworkManager* nm_;

  boost::asio::ip::tcp::endpoint endpoint_;

  ::boost::asio::ssl::context& ctx_;

  //bool enable_connection_aborted_ = true;

  // if < 0 => uses ::boost::asio::socket_base::max_listen_connections
  int max_listen_connections_ = -1;

  int max_sessions_count = 128;

  /**
   * I/O objects such as sockets and streams are not thread-safe. For efficiency, networking adopts
   * a model of using threads without explicit locking by requiring all access to I/O objects to be
   * performed within a strand.
   */
  //boost::asio::strand<boost::asio::io_context::executor_type> strand_;

  // TODO: mutex
  bool needClose_ = false;

  //std::unique_ptr<WS_LISTEN_MODE> mode_{std::make_unique<WS_LISTEN_MODE>(WS_LISTEN_MODE::BOTH)};
};

} // namespace ws
} // namespace net
} // namespace gloer

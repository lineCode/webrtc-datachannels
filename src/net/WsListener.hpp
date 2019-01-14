#pragma once

#include <beast/core/detail/config.hpp>
#include <beast/core/error.hpp>
#include <beast/http/error.hpp>
#include <beast/websocket/detail/error.hpp>
#include <boost/asio/basic_datagram_socket.hpp> // IWYU pragma: keep
#include <boost/asio/basic_streambuf.hpp>       // IWYU pragma: keep
#include <boost/asio/ip/tcp.hpp>
#include <memory>
#include <string>

namespace boost {
namespace asio {
class io_context;
} // namespace asio
} // namespace boost

namespace utils {
namespace net {
class SessionManager;
} // namespace net
} // namespace utils

namespace utils {
namespace net {

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

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

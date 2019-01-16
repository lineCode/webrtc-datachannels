#pragma once

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <memory>
#include <string>

namespace utils {
namespace net {

class NetworkManager;

// Accepts incoming connections and launches the sessions
class WsListener : public std::enable_shared_from_this<WsListener> {
  boost::asio::ip::tcp::acceptor acceptor_;
  boost::asio::ip::tcp::socket socket_;
  std::shared_ptr<std::string const> doc_root_;
  utils::net::NetworkManager* nm_;
  boost::asio::ip::tcp::endpoint endpoint_;

public:
  /*WsListener(boost::asio::io_context& ioc, utils::net::NetworkManager* nm)
      : acceptor_(ioc), socket_(ioc), nm_(nm) {}*/

  WsListener(boost::asio::io_context& ioc,
             const boost::asio::ip::tcp::endpoint& endpoint,
             std::shared_ptr<std::string const> doc_root,
             utils::net::NetworkManager* nm)
      : acceptor_(ioc), socket_(ioc), doc_root_(doc_root), nm_(nm),
        endpoint_(endpoint) {
    configureAcceptor();
  }

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
};

} // namespace net
} // namespace utils

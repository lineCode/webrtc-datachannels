#pragma once

#include <boost/asio/basic_datagram_socket.hpp> // IWYU pragma: keep
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/core/string.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/websocket/error.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <cstddef>
#include <string>
#include <vector>

class dispatch_queue;

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

// Echoes back all received WebSocket messages
class WsSession : public std::enable_shared_from_this<WsSession> {
  // TODO: private
  websocket::stream<tcp::socket> ws_;
  net::strand<net::io_context::executor_type> strand_;
  beast::multi_buffer buffer_;
  boost::asio::steady_timer timer_;
  bool busy_;
  /**
   * If you want to send more than one message at a time, you need to implement
   * your own write queue.
   * @see https://github.com/boostorg/beast/issues/1207
   **/
  std::vector<std::shared_ptr<std::string const>> queue_;
  std::shared_ptr<SessionManager> sm_;
  size_t ping_state_ = 0;
  const std::string id_;
  std::shared_ptr<dispatch_queue> receivedMessagesQueue_;

public:
  // Take ownership of the socket
  explicit WsSession(tcp::socket socket, std::shared_ptr<SessionManager> sm,
                     const std::string& id);

  std::string getId() const { return id_; }

  // Start the asynchronous operation
  void run();

  void on_session_fail(beast::error_code ec, char const* what);

  void on_control_callback(websocket::frame_type kind,
                           beast::string_view payload);

  // Called to indicate activity from the remote peer
  void onRemoteActivity();

  void on_accept(beast::error_code ec);

  // Called when the timer expires.
  void on_timer(beast::error_code ec);

  void do_read();

  /**
   * @brief starts async writing to client
   *
   * @param message message passed to client
   */
  void send(const std::string& ss);
  /*void send(const std::shared_ptr<const std::string>& ss);

  void send(const std::string& ss);*/

  void on_read(beast::error_code ec, std::size_t bytes_transferred);

  void on_write(beast::error_code ec, std::size_t bytes_transferred);

  void on_ping(beast::error_code ec);

  std::shared_ptr<dispatch_queue> getReceivedMessages() const;

  bool hasReceivedMessages() const;

  bool handleIncomingJSON();
};

} // namespace net
} // namespace utils

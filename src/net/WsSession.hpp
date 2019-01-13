#pragma once

#include "dispatch_queue.hpp"
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/bind_executor.hpp> // for asio
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
#include <cstddef> // for size_t
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
                     const std::string& id)
      : ws_(std::move(socket)), strand_(ws_.get_executor()), sm_(sm), id_(id),
        busy_(false), timer_(ws_.get_executor().context(),
                             (std::chrono::steady_clock::time_point::max)()) {
    receivedMessagesQueue_ = std::make_shared<dispatch_queue>(
        std::string{"WebSockets Server Dispatch Queue"}, 0);
    // TODO: SSL as in
    // https://github.com/vinniefalco/beast/blob/master/example/server-framework/main.cpp
    // Set options before performing the handshake.
    /**
     * Determines if outgoing message payloads are broken up into
     * multiple pieces.
     **/
    // ws_.auto_fragment(false);
    /**
     * Permessage-deflate allows messages to be compressed.
     **/
    beast::websocket::permessage_deflate pmd;
    pmd.client_enable = true;
    pmd.server_enable = true;
    pmd.compLevel = 3; /// Deflate compression level 0..9
    pmd.memLevel = 4;  // Deflate memory level, 1..9
    ws_.set_option(pmd);
    // ws.set_option(write_buffer_size{8192});
    /**
     * Set the maximum incoming message size option.
     * Message frame fields indicating a size that would bring the total message
     * size over this limit will cause a protocol failure.
     **/
    ws_.read_message_max(64 * 1024 * 1024);
    std::cout << "created WsSession #" << id_ << "\n";
  }

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

  std::shared_ptr<dispatch_queue> getReceivedMessages() const {
    // NOTE: Returned smart pointer by value to increment reference count
    return receivedMessagesQueue_;
  }

  bool hasReceivedMessages() const {
    return receivedMessagesQueue_.get()->empty();
  }

  bool handleIncomingJSON();
};

} // namespace net
} // namespace utils

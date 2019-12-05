#pragma once

/**
 * \note session does not have reconnect method - must create new session
 * \see https://www.boost.org/doc/libs/1_71_0/libs/beast/example/websocket/server/async/websocket_server_async.cpp
 **/

#include <net/http/SessionGUID.hpp>
#include <net/SessionBase.hpp>
//#include "net/SessionPair.hpp"
#include "net/core.hpp"
#include <api/datachannelinterface.h>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <cstddef>
#include <cstdint>
#include <folly/ProducerConsumerQueue.h>
#include <net/core.hpp>
#include <rapidjson/document.h>
#include <string>
#include <vector>
#include <webrtc/rtc_base/criticalsection.h>
#include <webrtc/rtc_base/sequenced_task_checker.h>
#include <net/NetworkManagerBase.hpp>
#include "net/ws/client/WSClientNetworkManager.hpp"
#include "net/ws/server/WSServerNetworkManager.hpp"

namespace gloer {
namespace algo {
class DispatchQueue;
} // namespace algo
} // namespace gloer

namespace gloer {
namespace net {

//class NetworkManager;

namespace http {
//class SessionGUID;
}

namespace wrtc {
class WRTCServer;
class WRTCSession;
} // namespace wrtc

namespace http {
/*class ServerConnectionManager;
class ServerSessionManager;
class ServerInputCallbacks;*/
class HTTPServerNetworkManager;
} // namespace http

} // namespace net
} // namespace gloer

namespace gloer {
namespace net {
namespace http {

/**
 * A class which represents a single connection
 * When this class is destroyed, the connection is closed.
 **/
class ServerSession
  : /*public SessionPair,*/ public SessionBase<http::SessionGUID>, public std::enable_shared_from_this<ServerSession> {
private:
  // NOTE: ProducerConsumerQueue must be created with a fixed maximum size
  // We use Queue per connection
  static const size_t MAX_SENDQUEUE_SIZE = 120;

public:
  ServerSession() = delete;

  // Take ownership of the socket
  explicit ServerSession(boost::asio::ip::tcp::socket&& socket,
    ::boost::asio::ssl::context& ctx,
    net::http::HTTPServerNetworkManager* nm,
    net::WSServerNetworkManager* ws_nm,
    const http::SessionGUID& id);

  ~ServerSession();

  void on_session_fail(boost::beast::error_code ec, char const* what);

  //void on_close(beast::error_code ec);

  void do_read();

  // bool handleIncomingJSON(std::shared_ptr<std::string> message) override;

  void send(const std::string& ss) /*override*/;

  bool isExpired() const /*override*/;

  bool isOpen() const /*override*/;

  void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);

  void on_write(boost::beast::error_code ec, std::size_t bytes_transferred, bool close);

  //void on_ping(boost::beast::error_code ec);

  //bool waitForConnect(std::size_t maxWait_ms) const;

  //void close() override;

  // void addGlobalSocketCount_s(uint32_t count); // RUN_ON(signaling_thread());

  // void subGlobalSocketCount_s(uint32_t count); // RUN_ON(signaling_thread());

private:
  static size_t MAX_IN_MSG_SIZE_BYTE;
  static size_t MAX_OUT_MSG_SIZE_BYTE;

  boost::beast::tcp_stream stream_;

  // The parser is stored in an optional container so we can
  // construct it from scratch it at the beginning of each new message.
  boost::optional<beast::http::request_parser<beast::http::string_body>> parser_;

  /**
   * I/O objects such as sockets and streams are not thread-safe. For efficiency, networking adopts
   * a model of using threads without explicit locking by requiring all access to I/O objects to be
   * performed within a strand.
   */
  //boost::asio::strand<boost::asio::io_context::executor_type> strand_;

  // resolver for connection as client
  //boost::asio::ip::tcp::resolver resolver_;

  /// \todo Use beast::flat_buffer or multibuffer ?
  boost::beast::multi_buffer buffer_;

  //boost::asio::steady_timer timer_;

  ::boost::asio::ssl::context& ctx_;

  // std::vector<std::shared_ptr<const std::string>> sendQueue_;
  /**
   * If you want to send more than one message at a time, you need to implement
   * your own write queue.
   * @see github.com/boostorg/beast/issues/1207
   *
   * @note ProducerConsumerQueue is a one producer and one consumer queue
   * without locks.
   **/
  //folly::ProducerConsumerQueue<std::shared_ptr<const std::string>> sendQueue_{MAX_SENDQUEUE_SIZE};
  //std::vector<std::shared_ptr<const std::string>> sendQueue_;

  net::http::HTTPServerNetworkManager* nm_;

  net::WSServerNetworkManager* ws_nm_;
};

} // namespace http
} // namespace net
} // namespace gloer

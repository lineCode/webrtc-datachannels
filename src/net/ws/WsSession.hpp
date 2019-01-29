#pragma once

#include "net/SessionBase.hpp"
#include "net/core.hpp"
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <cstddef>
#include <folly/ProducerConsumerQueue.h>
#include <net/core.hpp>
#include <string>
#include <vector>

namespace gloer {
namespace algo {
class DispatchQueue;
} // namespace algo
} // namespace gloer

namespace gloer {
namespace net {

class NetworkManager;

namespace ws {
class WSServer;
}

namespace wrtc {
class WRTCServer;
class WRTCSession;
class PCO;
} // namespace wrtc

} // namespace net
} // namespace gloer

namespace gloer {
namespace net {
namespace ws {

// NOTE: ProducerConsumerQueue must be created with a fixed maximum size
// We use Queue per connection, so it is for 1 client
constexpr size_t MAX_SENDQUEUE_SIZE = 16;

/**
 * A class which represents a single connection
 * When this class is destroyed, the connection is closed.
 **/
class WsSession : public SessionBase, public std::enable_shared_from_this<WsSession> {
public:
  // WsSession() {} // TODO

  // Take ownership of the socket
  explicit WsSession(boost::asio::ip::tcp::socket socket, NetworkManager* nm,
                     const std::string& id);

  ~WsSession();

  // Start the asynchronous operation
  void runAsServer();

  void on_session_fail(boost::beast::error_code ec, char const* what);

  void on_control_callback(boost::beast::websocket::frame_type kind,
                           boost::string_view payload); // TODO boost::string_view or
                                                        // std::string_view

  // Called to indicate activity from the remote peer
  void onRemoteActivity();

  void on_accept(boost::beast::error_code ec);

  // Called when the timer expires.
  void on_timer(boost::beast::error_code ec);

  void do_read();

  // bool handleIncomingJSON(std::shared_ptr<std::string> message) override;

  void send(const std::string& ss) override;

  bool isExpired() const override;

  void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);

  void on_write(boost::beast::error_code ec, std::size_t bytes_transferred);

  void on_ping(boost::beast::error_code ec);

  // std::shared_ptr<algo::DispatchQueue> getWRTCQueue() const;

  void pairToWRTCSession(std::shared_ptr<wrtc::WRTCSession> WRTCSession);

  /**
   * @brief returns WebRTC session paired with WebSocket session
   */
  std::weak_ptr<wrtc::WRTCSession> getWRTCSession() const;

  bool isOpen() const;

  bool fullyCreated() const { return isFullyCreated_; }

  bool waitForConnect(std::size_t maxWait_ms) const;

  void connectAsClient(const std::string& host, const std::string& port);

  void onClientResolve(beast::error_code ec, tcp::resolver::results_type results);

  void onClientConnect(beast::error_code ec);

  void onClientHandshake(beast::error_code ec);

  void runAsClient();

private:
  bool isFullyCreated_{false};

  bool isExpired_{false};

  static size_t MAX_IN_MSG_SIZE_BYTE;
  static size_t MAX_OUT_MSG_SIZE_BYTE;

  /**
   * The websocket::stream class template provides asynchronous and blocking message-oriented
   * functionality necessary for clients and servers to utilize the WebSocket protocol.
   * @note all asynchronous operations are performed within the same implicit or explicit strand.
   **/
  boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_;

  /**
   * I/O objects such as sockets and streams are not thread-safe. For efficiency, networking adopts
   * a model of using threads without explicit locking by requiring all access to I/O objects to be
   * performed within a strand.
   */
  boost::asio::strand<boost::asio::io_context::executor_type> strand_;

  // resolver for connection as client
  boost::asio::ip::tcp::resolver resolver_;

  boost::beast::multi_buffer recievedBuffer_;

  boost::asio::steady_timer timer_;

  bool isSendBusy_;

  // std::vector<std::shared_ptr<const std::string>> sendQueue_;
  /**
   * If you want to send more than one message at a time, you need to implement
   * your own write queue.
   * @see https://github.com/boostorg/beast/issues/1207
   *
   * @note ProducerConsumerQueue is a one producer and one consumer queue
   * without locks.
   **/
  folly::ProducerConsumerQueue<std::shared_ptr<const std::string>> sendQueue_{MAX_SENDQUEUE_SIZE};

  NetworkManager* nm_;

  uint32_t pingState_ = 0;

  // NOTE: may be empty!
  // TODO: weak ptr?
  std::weak_ptr<wrtc::WRTCSession> wrtcSession_;
};

} // namespace ws
} // namespace net
} // namespace gloer

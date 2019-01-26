#pragma once

#include "net/SessionBase.hpp"
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/experimental/core/ssl_stream.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/config.hpp>
#include <boost/make_unique.hpp>
#include <cstddef>
#include <cstdlib>
#include <folly/ProducerConsumerQueue.h>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace gloer {
namespace algo {
class DispatchQueue;
} // namespace algo
} // namespace gloer

namespace gloer {
namespace net {

// NOTE: ProducerConsumerQueue must be created with a fixed maximum size
// We use Queue per connection, so it is for 1 client
constexpr size_t maxSendQueueElems = 16;

class NetworkManager;
class WRTCServer;
class WRTCSession;
class PCO;

// Echoes back all received WebSocket messages
class WsSession : public SessionBase, public std::enable_shared_from_this<WsSession> {
public:
  // WsSession() {} // TODO

  // Take ownership of the socket
  explicit WsSession(boost::beast::ssl_stream<boost::asio::ip::tcp::socket> socketStream,
                     NetworkManager* nm, const std::string& id/*,
                     boost::asio::ssl::context& ctx, boost::beast::flat_buffer buffer*/);

  ~WsSession();

  // Start the asynchronous operation
  void run();

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

  bool handleIncomingJSON(std::shared_ptr<std::string> message) override;

  void send(std::shared_ptr<std::string> ss) override;

  void send(const std::string& ss) override;

  void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);

  void on_write(boost::beast::error_code ec, std::size_t bytes_transferred);

  void on_ping(boost::beast::error_code ec);

  // std::shared_ptr<algo::DispatchQueue> getWRTCQueue() const;

  void pairToWRTCSession(std::shared_ptr<WRTCSession> WRTCSession);

  /**
   * @brief returns WebRTC session paired with WebSocket session
   */
  std::weak_ptr<WRTCSession> getWRTCSession() const;

  bool isOpen() const;

  bool fullyCreated() const { return isFullyCreated_; }

  void on_close(boost::system::error_code ec);

private:
  bool isFullyCreated_{false};

  /**
   * 16 Kbyte for the highest throughput, while also being the most portable one
   * @see https://viblast.com/blog/2015/2/5/webrtc-data-channel-message-size/
   **/
  static constexpr size_t maxReceiveMsgSizebyte = 16 * 1024;
  static constexpr size_t maxSendMsgSizebyte = 16 * 1024;

  /**
   * The websocket::stream class template provides asynchronous and blocking message-oriented
   * functionality necessary for clients and servers to utilize the WebSocket protocol.
   * @note all asynchronous operations are performed within the same implicit or explicit strand.
   **/
  // boost::beast::websocket::stream<boost::asio::ip::tcp::socket> wsStream_;
  boost::beast::websocket::stream<boost::beast::ssl_stream<boost::asio::ip::tcp::socket>> wsStream_;

  /**
   * I/O objects such as sockets and streams are not thread-safe. For efficiency, networking adopts
   * a model of using threads without explicit locking by requiring all access to I/O objects to be
   * performed within a strand.
   */
  boost::asio::strand<boost::asio::io_context::executor_type> strand_;

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
  folly::ProducerConsumerQueue<std::shared_ptr<const std::string>> sendQueue_{maxSendQueueElems};

  NetworkManager* nm_;

  uint32_t pingState_ = 0;

  // NOTE: may be empty!
  // TODO: weak ptr?
  std::weak_ptr<WRTCSession> wrtcSession_;

  bool close_ = false;
};

} // namespace net
} // namespace gloer

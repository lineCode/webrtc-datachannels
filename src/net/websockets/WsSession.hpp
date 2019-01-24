#pragma once

#include "net/SessionI.hpp"
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <cstddef>
#include <folly/ProducerConsumerQueue.h>
#include <string>
#include <vector>

namespace gloer {
namespace algo {
class DispatchQueue;
} // namespace algo
} // namespace utils

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
class WsSession : public SessionI, public std::enable_shared_from_this<WsSession> {
public:
  // WsSession() {} // TODO

  // Take ownership of the socket
  explicit WsSession(boost::asio::ip::tcp::socket socket, NetworkManager* nm,
                     const std::string& id);

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

private:
  bool isFullyCreated_{false};

  /**
   * 16 Kbyte for the highest throughput, while also being the most portable one
   * @see https://viblast.com/blog/2015/2/5/webrtc-data-channel-message-size/
   **/
  static constexpr size_t maxReceiveMsgSizebyte = 16 * 1024;
  static constexpr size_t maxSendMsgSizebyte = 16 * 1024;

  boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_;

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

  size_t pingState_ = 0;

  // NOTE: may be empty!
  // TODO: weak ptr?
  std::weak_ptr<WRTCSession> wrtcSession_;
};

} // namespace net
} // namespace utils

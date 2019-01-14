#include "net/WsSession.hpp"
#include "dispatch_queue.hpp"
#include "net/SessionManager.hpp"
#include <boost/asio/basic_socket.hpp>
#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/detail/impl/epoll_reactor.hpp>
#include <boost/asio/detail/impl/service_registry.hpp>
#include <boost/asio/detail/impl/strand_executor_service.ipp>
#include <boost/asio/error.hpp>
#include <boost/asio/impl/io_context.hpp>
#include <boost/assert.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/detail/buffers_range_adaptor.hpp>
#include <boost/beast/core/impl/buffers_cat.hpp>
#include <boost/beast/core/impl/buffers_prefix.hpp>
#include <boost/beast/core/impl/buffers_suffix.hpp>
#include <boost/beast/core/impl/flat_static_buffer.hpp>
#include <boost/beast/core/impl/handler_ptr.hpp>
#include <boost/beast/core/impl/multi_buffer.hpp>
#include <boost/beast/core/impl/static_buffer.hpp>
#include <boost/beast/core/impl/static_string.hpp>
#include <boost/beast/core/impl/string_param.hpp>
#include <boost/beast/http/detail/basic_parsed_list.hpp>
#include <boost/beast/http/impl/basic_parser.ipp>
#include <boost/beast/http/impl/fields.ipp>
#include <boost/beast/http/impl/message.ipp>
#include <boost/beast/http/impl/parser.ipp>
#include <boost/beast/http/impl/serializer.ipp>
#include <boost/beast/http/impl/status.ipp>
#include <boost/beast/http/rfc7230.hpp>
#include <boost/beast/websocket/error.hpp>
#include <boost/beast/websocket/impl/accept.ipp>
#include <boost/beast/websocket/impl/error.ipp>
#include <boost/beast/websocket/impl/ping.ipp>
#include <boost/beast/websocket/impl/read.ipp>
#include <boost/beast/websocket/impl/stream.ipp>
#include <boost/beast/websocket/impl/write.ipp>
#include <boost/beast/websocket/option.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/intrusive/detail/list_iterator.hpp>
#include <boost/intrusive/detail/tree_iterator.hpp>
#include <boost/system/error_code.hpp>
#include <boost/utility/string_view.hpp>
#include <chrono>
#include <cstdint>
#include <ext/alloc_traits.h>
#include <functional>
#include <inttypes.h>
#include <iostream>
#include <map>
#include <memory>
#include <new>
#include <rapidjson/document.h>
#include <rapidjson/error/error.h>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>

/**
 * The amount of time to wait in seconds, before sending a websocket 'ping'
 * message. Ping messages are used to determine if the remote end of the
 * connection is no longer available.
 **/
constexpr unsigned long WS_PING_FREQUENCY_SEC = 15;

/**
 * Type field stores uint32_t -> [0-4294967295] -> max 10 digits
 **/
constexpr unsigned long UINT32_FIELD_MAX_LEN = 10;

constexpr size_t PING_STATE_ALIVE = 0;
constexpr size_t PING_STATE_SENDING = 1;
constexpr size_t PING_STATE_SENT = 2;

constexpr size_t SEND_QUEUE_LIMIT = 1024; // TODO: reserve to SEND_QUEUE_LIMIT?

namespace utils {
namespace net {

struct NetworkOperation {
  NetworkOperation(uint32_t operationCode, const std::string& operationName)
      : operationCode(operationCode), operationName(operationName),
        operationCodeStr(std::to_string(operationCode)) {}

  NetworkOperation(uint32_t operationCode)
      : operationCode(operationCode), operationName(""),
        operationCodeStr(std::to_string(operationCode)) {}

  const uint32_t operationCode;
  const std::string operationCodeStr;
  /**
   * operationName usefull for logging
   * NOTE: operationName may be empty
   **/
  const std::string operationName;

  bool operator<(const NetworkOperation& rhs) const {
    return operationCode < rhs.operationCode;
  }

  bool operator<(const uint32_t& rhsOperationCode) const {
    return operationCode < rhsOperationCode;
  }
};

const NetworkOperation PING_OPERATION = NetworkOperation(0, "PING");
const NetworkOperation CANDIDATE_OPERATION = NetworkOperation(0, "CANDIDATE");
const NetworkOperation OFFER_OPERATION = NetworkOperation(0, "OFFER");
const NetworkOperation ANSWER_OPERATION = NetworkOperation(0, "ANSWER");
// TODO: handle all opcodes

using NetworkOperationCallback =
    std::function<void(WsSession* clientSession,
                       std::shared_ptr<beast::multi_buffer> messageBuffer)>;

void pingCallback(WsSession* clientSession,
                  std::shared_ptr<beast::multi_buffer> messageBuffer) {
  const std::string incomingStr =
      beast::buffers_to_string(messageBuffer->data());
  std::cout << std::this_thread::get_id() << ":"
            << "receivedMessagesQ_->dispatch incomingMsg=" << incomingStr
            << std::endl;
  // send same message back (ping-pong)
  clientSession->send(beast::buffers_to_string(messageBuffer->data()));
}

std::map<NetworkOperation, NetworkOperationCallback> networkOperationCallbacks{
    {PING_OPERATION, &pingCallback},
    // {PING_OPERATION, &pingCallback},
};

void WsSession::on_session_fail(beast::error_code ec, char const* what) {
  std::cerr << "WsSession: " << what << " : " << ec.message() << "\n";
  // const std::string wsGuid = boost::lexical_cast<std::string>(getId());
  sm_->unregisterSession(getId());
}

WsSession::WsSession(tcp::socket socket, std::shared_ptr<SessionManager> sm,
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

// Start the asynchronous operation
void WsSession::run() {
  std::cout << "WS session run\n";

  // Set the control callback. This will be called
  // on every incoming ping, pong, and close frame.
  ws_.control_callback(std::bind(&WsSession::on_control_callback, this,
                                 std::placeholders::_1, std::placeholders::_2));

  // Run the timer. The timer is operated
  // continuously, this simplifies the code.
  on_timer({});

  // Set the timer
  timer_.expires_after(std::chrono::seconds(WS_PING_FREQUENCY_SEC));

  // Accept the websocket handshake
  // Start reading and responding to a WebSocket HTTP Upgrade request.
  ws_.async_accept(net::bind_executor(
      strand_, std::bind(&WsSession::on_accept, shared_from_this(),
                         std::placeholders::_1)));
}

void WsSession::on_control_callback(websocket::frame_type kind,
                                    beast::string_view payload) {
  std::cout << "WS on_control_callback\n";
  boost::ignore_unused(kind, payload);

  // Note that there is activity
  onRemoteActivity();
}

// Called to indicate activity from the remote peer
void WsSession::onRemoteActivity() {
  // Note that the connection is alive
  ping_state_ = PING_STATE_ALIVE;

  // Set the timer
  timer_.expires_after(std::chrono::seconds(WS_PING_FREQUENCY_SEC));
}

void WsSession::on_accept(beast::error_code ec) {
  std::cout << "WS session on_accept\n";

  // Happens when the timer closes the socket
  if (ec == net::error::operation_aborted) {
    std::cout << "WS on_accept: operation_aborted\n";
    return;
  }

  if (ec)
    return on_session_fail(ec, "accept");

  // Read a message
  do_read();
}

std::shared_ptr<dispatch_queue> WsSession::getReceivedMessages() const {
  // NOTE: Returned smart pointer by value to increment reference count
  return receivedMessagesQueue_;
}

// Called after a ping is sent.
void WsSession::on_ping(beast::error_code ec) {
  // Happens when the timer closes the socket
  if (ec == net::error::operation_aborted) {
    std::cout << "WsSession on_ping: net::error::operation_aborted\n";
    return;
  }

  if (ec)
    return on_session_fail(ec, "ping");

  // Note that the ping was sent.
  if (ping_state_ == PING_STATE_SENDING) {
    ping_state_ = PING_STATE_SENT;
  } else {
    // ping_state_ could have been set to 0
    // if an incoming control frame was received
    // at exactly the same time we sent a ping.
    BOOST_ASSERT(ping_state_ == PING_STATE_ALIVE);
  }
}

// Called when the timer expires.
void WsSession::on_timer(beast::error_code ec) {
  std::cerr << "expired WsSession::on_timer\n";

  if (ec && ec != net::error::operation_aborted)
    return on_session_fail(ec, "timer");

  // See if the timer really expired since the deadline may have moved.
  if (timer_.expiry() <= std::chrono::steady_clock::now()) {
    // If this is the first time the timer expired,
    // send a ping to see if the other end is there.
    if (ws_.is_open() && ping_state_ == PING_STATE_ALIVE) {
      // Note that we are sending a ping
      ping_state_ = PING_STATE_SENDING;

      // Set the timer
      timer_.expires_after(std::chrono::seconds(WS_PING_FREQUENCY_SEC));

      // Now send the ping
      ws_.async_ping(
          {}, net::bind_executor(strand_, std::bind(&WsSession::on_ping,
                                                    shared_from_this(),
                                                    std::placeholders::_1)));
    } else {
      // The timer expired while trying to handshake,
      // or we sent a ping and it never completed or
      // we never got back a control frame, so close.

      // Closing the socket cancels all outstanding operations. They
      // will complete with net::error::operation_aborted
      std::cout << "The timer expired while trying to handshake, or we sent a "
                   "ping and it never completed or we never got back a control "
                   "frame, so close.\n";
      ws_.next_layer().shutdown(tcp::socket::shutdown_both, ec);
      ws_.next_layer().close(ec);
      sm_->unregisterSession(getId());
      return;
    }
  }

  // Wait on the timer
  timer_.async_wait(net::bind_executor(
      strand_, std::bind(&WsSession::on_timer, shared_from_this(),
                         std::placeholders::_1)));
}

void WsSession::do_read() {
  std::cout << "WS session do_read\n";

  // Set the timer
  // timer_.expires_after(std::chrono::seconds(WS_PING_FREQUENCY_SEC));

  // Read a message into our buffer
  ws_.async_read(buffer_,
                 net::bind_executor(strand_, std::bind(&WsSession::on_read,
                                                       shared_from_this(),
                                                       std::placeholders::_1,
                                                       std::placeholders::_2)));
}

void WsSession::on_read(beast::error_code ec, std::size_t bytes_transferred) {
  std::cout << "WS session on_read\n";

  boost::ignore_unused(bytes_transferred);

  // Happens when the timer closes the socket
  if (ec == net::error::operation_aborted) {
    std::cout << "WsSession on_read: net::error::operation_aborted\n";
    return;
  }

  // This indicates that the session was closed
  if (ec == websocket::error::closed)
    return;

  if (ec)
    on_session_fail(ec, "read");

  // Note that there is activity
  // onRemoteActivity();

  // std::cout << "buffer: " << beast::buffers_to_string(buffer_.data()) <<
  // "\n"; send(beast::buffers_to_string(buffer_.data())); // ??????

  if (!receivedMessagesQueue_) {
    std::cout << "WsSession::on_read invalid receivedMessagesQ_" << std::endl;
  }

  // add incoming message callback into queue
  // TODO: use protobuf
  handleIncomingJSON();

  // TODO: remove
  /*if (type == "ping") {
    beast::multi_buffer buffer_copy = buffer_;
    receivedMessagesQueue_->dispatch([this, buffer_copy] {
      const std::string incomingStr =
          beast::buffers_to_string(buffer_copy.data());
      std::cout << std::this_thread::get_id() << ":"
                << "receivedMessagesQ_->dispatch incomingMsg=" << incomingStr
                << std::endl;
      // send same message back (ping-pong)
      send(beast::buffers_to_string(buffer_copy.data()));
    });
  } else {
    std::cout << "WsSession::on_read: ignored invalid message " << std::endl;
  }*/

  // Clear the buffer
  buffer_.consume(buffer_.size());

  // Do another read
  do_read();
}

bool WsSession::hasReceivedMessages() const {
  return receivedMessagesQueue_.get()->empty();
}

/**
 * Add message to queue for further processing
 * Returs true if message can be processed
 **/
bool WsSession::handleIncomingJSON() {
  const std::string incomingStr = beast::buffers_to_string(buffer_.data());
  auto sharedBuffer = std::make_shared<beast::multi_buffer>(buffer_);
  // parse incoming message
  rapidjson::Document message_object;
  rapidjson::ParseResult result = message_object.Parse(incomingStr.c_str());
  if (!result || !message_object.IsObject() ||
      !message_object.HasMember("type")) {
    std::cout << "WsSession::on_read: ignored invalid message without type";
    return false;
  }
  // Probably should do some error checking on the JSON object.
  std::string typeStr = message_object["type"].GetString();
  if (typeStr.empty() || typeStr.length() > UINT32_FIELD_MAX_LEN) {
    std::cout << "WsSession::on_read: ignored invalid message with invalid "
                 "type field\n";
  }
  // TODO str -> uint32_t: to UTILS file
  // see https://stackoverflow.com/a/5745454/10904212
  uint32_t type; // NOTE: on change: don`t forget about UINT32_FIELD_MAX_LEN
  sscanf(typeStr.c_str(), "%" SCNu32, &type);
  auto it = networkOperationCallbacks.find(NetworkOperation(type));
  // if a callback is registered for event, add it to queue
  if (it != networkOperationCallbacks.end()) {
    NetworkOperationCallback callback = it->second;
    auto callbackBind = std::bind(callback, this, sharedBuffer);
    receivedMessagesQueue_->dispatch(callbackBind);
  } else {
    std::cout << "WsSession::on_read: ignored invalid message with type "
              << typeStr << std::endl;
    return false;
  }

  return true;
}

void WsSession::on_write(beast::error_code ec, std::size_t bytes_transferred) {
  std::cout << "WS session on_write\n";
  boost::ignore_unused(bytes_transferred);

  // Happens when the timer closes the socket
  if (ec == net::error::operation_aborted) {
    std::cout << "WsSession on_write: net::error::operation_aborted\n";
    return;
  }

  if (ec) {
    std::cout << "WsSession on_write: ec\n";
    return on_session_fail(ec, "write");
  }

  // Remove the string from the queue
  queue_.erase(queue_.begin());

  if (!queue_.empty()) {
    std::cout << "write buffer: " << *queue_.front() << "\n";

    ws_.async_write(
        net::buffer(*queue_.front()),
        net::bind_executor(
            strand_, std::bind(&WsSession::on_write, shared_from_this(),
                               std::placeholders::_1, std::placeholders::_2)));
  } else {
    std::cout << "write queue_.empty()\n";
    busy_ = false;
  }
}

/**
 * @brief starts async writing to client
 *
 * @param message message passed to client
 */
void WsSession::send(const std::string& ss) {
  auto const ssShared = std::make_shared<std::string const>(std::move(ss));

  if (queue_.size() < SEND_QUEUE_LIMIT) {
    queue_.push_back(ssShared);
  } else {
    std::cerr << "queue_.size() > SEND_QUEUE_LIMIT\n";
  }

  // Are we already writing?
  if (queue_.size() > 1) {
    std::cout << "queue_.size() > 1\n";
    return;
  }

  if (!busy_ && !queue_.empty()) {
    busy_ = true;
    // We are not currently writing, so send this immediately
    ws_.async_write(
        net::buffer(*queue_.front()),
        net::bind_executor(
            strand_, std::bind(&WsSession::on_write, shared_from_this(),
                               std::placeholders::_1, std::placeholders::_2)));
  }
}

} // namespace net
} // namespace utils

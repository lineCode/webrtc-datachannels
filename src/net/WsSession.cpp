#include "net/WsSession.hpp"
#include "net/SessionManager.hpp"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <functional>
#include <iostream>
#include <map>
#include <string>

#include <inttypes.h>
#include <stdio.h>

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

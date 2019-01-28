#include "net/websockets/WsSession.hpp" // IWYU pragma: associated
#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include "net/webrtc/WRTCServer.hpp"
#include "net/websockets/WsServer.hpp"
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/system/error_code.hpp>
#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <enum.h>
#include <functional>
#include <iostream>
#include <map>
#include <new>
#include <rapidjson/document.h>
#include <rapidjson/error/error.h>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

/**
 * The amount of time to wait in seconds, before sending a websocket 'ping'
 * message. Ping messages are used to determine if the remote end of the
 * connection is no longer available.
 **/
constexpr unsigned long WS_PING_FREQUENCY_SEC = 60;

namespace {

BETTER_ENUM(PING_STATE, uint32_t, ALIVE, SENDING, SENT, TOTAL)

}

namespace gloer {
namespace net {

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

void WsSession::on_session_fail(beast::error_code ec, char const* what) {
  LOG(WARNING) << "WsSession: " << what << " : " << ec.message();
  // const std::string wsGuid = boost::lexical_cast<std::string>(getId());
  std::string copyId = getId();
  nm_->getWS()->unregisterSession(copyId);
}

// @note tcp::socket socket represents the local end of a connection between two peers
WsSession::WsSession(tcp::socket socket, NetworkManager* nm, const std::string& id)
    : SessionBase(id), ws_(std::move(socket)), strand_(ws_.get_executor()), nm_(nm),
      timer_(ws_.get_executor().context(), (std::chrono::steady_clock::time_point::max)()),
      isSendBusy_(false) {

  receivedMessagesQueue_ =
      std::make_shared<algo::DispatchQueue>(std::string{"WebSockets Server Dispatch Queue"}, 0);
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
  LOG(INFO) << "created WsSession #" << id_;
}

WsSession::~WsSession() {
  LOG(INFO) << "~WsSession";
  /*if (receivedMessagesQueue_ && receivedMessagesQueue_.get())
    receivedMessagesQueue_.reset();
  // sendQueue_.
  //nm_->getWS()->unregisterSession(id_);*/
}

size_t WsSession::MAX_IN_MSG_SIZE_BYTE = 16 * 1024;
size_t WsSession::MAX_OUT_MSG_SIZE_BYTE = 16 * 1024;

// Start the asynchronous operation
void WsSession::run() {
  LOG(INFO) << "WS session run";

  /*if (!isOpen()) {
    LOG(WARNING) << "!ws_.is_open()";
    return;
  }*/

  // Set the control callback. This will be called
  // on every incoming ping, pong, and close frame.
  ws_.control_callback(std::bind(&WsSession::on_control_callback, this, std::placeholders::_1,
                                 std::placeholders::_2));

  // Run the timer. The timer is operated
  // continuously, this simplifies the code.
  on_timer({});

  // Set the timer
  timer_.expires_after(std::chrono::seconds(WS_PING_FREQUENCY_SEC));

  /*if (!isOpen()) {
    LOG(WARNING) << "!ws_.is_open()";
    return;
  }*/

  // Accept the websocket handshake
  // Start reading and responding to a WebSocket HTTP Upgrade request.
  ws_.async_accept(net::bind_executor(
      strand_, std::bind(&WsSession::on_accept, shared_from_this(), std::placeholders::_1)));
}

void WsSession::on_control_callback(websocket::frame_type kind, beast::string_view payload) {
  // LOG(INFO) << "WS on_control_callback";
  boost::ignore_unused(kind, payload);

  // Note that there is activity
  onRemoteActivity();
}

// Called to indicate activity from the remote peer
void WsSession::onRemoteActivity() {
  // Note that the connection is alive
  pingState_ = PING_STATE::ALIVE;

  // Set the timer
  timer_.expires_after(std::chrono::seconds(WS_PING_FREQUENCY_SEC));
}

void WsSession::on_accept(beast::error_code ec) {
  LOG(INFO) << "WS session on_accept";

  // Happens when the timer closes the socket
  if (ec == net::error::operation_aborted) {
    LOG(WARNING) << "WsSession on_accept ec:" << ec.message();
    return;
  }

  if (ec)
    return on_session_fail(ec, "accept");

  isFullyCreated_ = true; // TODO

  // Read a message
  do_read();
}

// Called after a ping is sent.
void WsSession::on_ping(beast::error_code ec) {
  // Happens when the timer closes the socket
  if (ec == net::error::operation_aborted) {
    LOG(WARNING) << "WsSession on_ping ec:" << ec.message();
    return;
  }

  if (ec) {
    LOG(WARNING) << "WsSession on_ping ec:" << ec.message();
    return on_session_fail(ec, "ping");
  }

  // Note that the ping was sent.
  if (pingState_ == PING_STATE::SENDING) {
    pingState_ = PING_STATE::SENT;
  } else {
    // ping_state_ could have been set to 0
    // if an incoming control frame was received
    // at exactly the same time we sent a ping.
    BOOST_ASSERT(pingState_ == PING_STATE::ALIVE);
  }
}

// Called when the timer expires.
void WsSession::on_timer(beast::error_code ec) {
  // LOG(INFO) << "WsSession::on_timer";

  if (ec && ec != net::error::operation_aborted) {
    LOG(WARNING) << "WsSession on_timer ec:" << ec.message();
    return on_session_fail(ec, "timer");
  }

  // See if the timer really expired since the deadline may have moved.
  if (timer_.expiry() <= std::chrono::steady_clock::now()) {
    // If this is the first time the timer expired,
    // send a ping to see if the other end is there.
    if (isOpen() && pingState_ == PING_STATE::ALIVE) {
      // Note that we are sending a ping
      pingState_ = PING_STATE::SENDING;

      // Set the timer
      timer_.expires_after(std::chrono::seconds(WS_PING_FREQUENCY_SEC));

      // Now send the ping
      ws_.async_ping({},
                     net::bind_executor(strand_, std::bind(&WsSession::on_ping, shared_from_this(),
                                                           std::placeholders::_1)));
    } else {
      // The timer expired while trying to handshake,
      // or we sent a ping and it never completed or
      // we never got back a control frame, so close.

      // Closing the socket cancels all outstanding operations. They
      // will complete with net::error::operation_aborted
      LOG(INFO) << "The timer expired while trying to handshake, or we sent a "
                   "ping and it never completed or we never got back a control "
                   "frame, so close.";
      LOG(INFO) << "on_timer: total ws sessions: " << nm_->getWS()->getSessionsCount();
      ws_.next_layer().shutdown(tcp::socket::shutdown_both, ec);
      ws_.next_layer().close(ec);
      std::string copyId = getId();
      nm_->getWS()->unregisterSession(copyId);
      isExpired_ = true;
      return;
    }
  }

  // Wait on the timer
  timer_.async_wait(net::bind_executor(
      strand_, std::bind(&WsSession::on_timer, shared_from_this(), std::placeholders::_1)));
}

void WsSession::do_read() {
  // LOG(INFO) << "WS session do_read";

  // Set the timer
  // timer_.expires_after(std::chrono::seconds(WS_PING_FREQUENCY_SEC));

  /*if (!isOpen()) {
    LOG(WARNING) << "!ws_.is_open()";
    return;
  }*/

  // Read a message into our buffer
  ws_.async_read(
      recievedBuffer_,
      net::bind_executor(strand_, std::bind(&WsSession::on_read, shared_from_this(),
                                            std::placeholders::_1, std::placeholders::_2)));
}

void WsSession::on_read(beast::error_code ec, std::size_t bytes_transferred) {
  // LOG(INFO) << "WS session on_read";

  boost::ignore_unused(bytes_transferred);

  // Happens when the timer closes the socket
  if (ec == net::error::operation_aborted) {
    LOG(WARNING) << "WsSession on_read: net::error::operation_aborted";
    return;
  }

  // This indicates that the session was closed
  if (ec == websocket::error::closed) {
    LOG(WARNING) << "WsSession on_read ec:" << ec.message();
    return; // on_session_fail(ec, "write");
  }

  if (ec)
    on_session_fail(ec, "read");

  // Note that there is activity
  // onRemoteActivity();

  // LOG(INFO) << "buffer: " <<
  // beast::buffers_to_string(recieved_buffer_.data());
  // send(beast::buffers_to_string(recieved_buffer_.data())); // ??????

  if (!receivedMessagesQueue_ || !receivedMessagesQueue_.get()) {
    LOG(WARNING) << "WsSession::on_read: invalid receivedMessagesQueue_ ";
    return;
  }

  if (!recievedBuffer_.size()) {
    LOG(WARNING) << "WsSession::on_read: empty messageBuffer";
    return;
  }

  if (recievedBuffer_.size() > MAX_IN_MSG_SIZE_BYTE) {
    LOG(WARNING) << "WsSession::on_read: Too big messageBuffer of size " << recievedBuffer_.size();
    return;
  }

  // add incoming message callback into queue
  // TODO: use protobuf
  auto sharedBuffer =
      std::make_shared<std::string>(beast::buffers_to_string(recievedBuffer_.data()));
  handleIncomingJSON(sharedBuffer);

  // TODO: remove
  /*if (type == "ping") {
    beast::multi_buffer buffer_copy = recieved_buffer_;
    receivedMessagesQueue_->dispatch([this, buffer_copy] {
      const std::string incomingStr =
          beast::buffers_to_string(buffer_copy.data());
      LOG(INFO) << std::this_thread::get_id() << ":"
                << "receivedMessagesQ_->dispatch incomingMsg=" << incomingStr;
      // send same message back (ping-pong)
      send(beast::buffers_to_string(buffer_copy.data()));
    });
  } else {
    LOG(WARNING) << "WsSession::on_read: ignored invalid message ";
  }*/

  // Clear the buffer
  recievedBuffer_.consume(recievedBuffer_.size());

  // Do another read
  do_read();
}

/*std::shared_ptr<algo::DispatchQueue> WsSession::getWRTCQueue() const {
  return getWRTC()->getWRTCQueue();
}*/

void WsSession::pairToWRTCSession(std::shared_ptr<WRTCSession> WRTCSession) {
  LOG(INFO) << "pairToWRTCSessionn...";
  if (!WRTCSession) {
    LOG(WARNING) << "pairToWRTCSession: Invalid WRTCSession";
    return;
  }
  wrtcSession_ = WRTCSession;
}

std::weak_ptr<WRTCSession> WsSession::getWRTCSession() const {
  if (!wrtcSession_.lock()) {
    LOG(WARNING) << "getWRTCSession: Invalid wrtcSession_";
    return wrtcSession_;
  }
  return wrtcSession_;
}

bool WsSession::isOpen() const { return ws_.is_open(); }

/**
 * Add message to queue for further processing
 * Returs true if message can be processed
 **/
bool WsSession::handleIncomingJSON(std::shared_ptr<std::string> message) {
  if (!message || !message.get()) {
    LOG(WARNING) << "WRTCSession::handleIncomingJSON: invalid message";
    return false;
  }

  // parse incoming message
  rapidjson::Document message_object;
  rapidjson::ParseResult result = message_object.Parse(message->c_str());
  // LOG(INFO) << "incomingStr: " << message->c_str();
  if (!result || !message_object.IsObject() || !message_object.HasMember("type")) {
    LOG(WARNING) << "WsSession::handleIncomingJSON: ignored invalid message without type";
    return false;
  }
  // Probably should do some error checking on the JSON object.
  std::string typeStr = message_object["type"].GetString();
  if (typeStr.empty() || typeStr.length() > UINT32_FIELD_MAX_LEN) {
    LOG(WARNING) << "WsSession::handleIncomingJSON: ignored invalid message with invalid "
                    "type field";
  }
  const auto& callbacks = nm_->getWS()->getOperationCallbacks().getCallbacks();

  const WsNetworkOperation wsNetworkOperation =
      static_cast<algo::WS_OPCODE>(algo::Opcodes::wsOpcodeFromStr(typeStr));
  const auto itFound = callbacks.find(wsNetworkOperation);
  // if a callback is registered for event, add it to queue
  if (itFound != callbacks.end()) {
    WsNetworkOperationCallback callback = itFound->second;
    algo::DispatchQueue::dispatch_callback callbackBind = std::bind(callback, this, nm_, message);
    if (!receivedMessagesQueue_ || !receivedMessagesQueue_.get()) {
      LOG(WARNING) << "WsSession::handleIncomingJSON: invalid receivedMessagesQueue_ ";
      return false;
    }
    receivedMessagesQueue_->dispatch(callbackBind);

    /*LOG(WARNING) << "WsSession::handleIncomingJSON: receivedMessagesQueue_->sizeGuess() "
                 << receivedMessagesQueue_->sizeGuess();*/
  } else {
    LOG(WARNING) << "WsSession::handleIncomingJSON: ignored invalid message with type " << typeStr;
    return false;
  }

  return true;
}

void WsSession::on_write(beast::error_code ec, std::size_t bytes_transferred) {
  // LOG(INFO) << "WS session on_write";
  boost::ignore_unused(bytes_transferred);

  // Happens when the timer closes the socket
  if (ec == net::error::operation_aborted) {
    LOG(WARNING) << "WsSession on_write: net::error::operation_aborted: " << ec.message();
    return;
  }

  if (ec) {
    LOG(WARNING) << "WsSession on_write: ec";
    return on_session_fail(ec, "write");
  }

  if (!isOpen()) {
    LOG(WARNING) << "!ws_.is_open()";
    return;
  }

  if (!sendQueue_.isEmpty()) {
    // Remove the already written string from the queue
    // sendQueue_.erase(sendQueue_.begin());
    sendQueue_.popFront();
  }

  if (!sendQueue_.isEmpty()) {

    if (!sendQueue_.frontPtr() || !sendQueue_.frontPtr()->get()) {
      LOG(WARNING) << "ws: invalid sendQueue_.frontPtr()";
      return;
    }

    // std::shared_ptr<const std::string> dp = *(sendQueue_.frontPtr());
    std::shared_ptr<const std::string> dp;
    sendQueue_.read(dp);

    if (!dp || !dp.get()) {
      LOG(WARNING) << "invalid sendQueue_.front()) ";
      return;
    }

    // LOG(INFO) << "write buffer: " << *dp;

    // This controls whether or not outgoing message opcodes are set to binary or text.
    ws_.text(true);
    ws_.async_write(
        net::buffer(*dp),
        net::bind_executor(strand_, std::bind(&WsSession::on_write, shared_from_this(),
                                              std::placeholders::_1, std::placeholders::_2)));
  } else {
    LOG(INFO) << "write send_queue_.empty()";
    isSendBusy_ = false;
  }
}

// TODO: support bytestream
void WsSession::send(std::shared_ptr<std::string> ss) {
  if (!ss || !ss.get()) {
    LOG(WARNING) << "WRTCSession::send: Invalid messageBuffer";
    return;
  }
  send(ss.get()->c_str());
}

/**
 * @brief starts async writing to client
 *
 * @param message message passed to client
 */
void WsSession::send(const std::string& ss) {
  // LOG(WARNING) << "WsSession::send:" << ss;
  std::shared_ptr<const std::string> ssShared =
      std::make_shared<const std::string>(ss); // TODO: std::move

  if (!ssShared || !ssShared.get() || ssShared->empty()) {
    LOG(WARNING) << "WsSession::send: empty messageBuffer";
    return;
  }

  if (ssShared->size() > MAX_OUT_MSG_SIZE_BYTE) {
    LOG(WARNING) << "WsSession::send: Too big messageBuffer of size " << ssShared->size();
    return;
  }

  // TODO: use folly fixed size queue
  if (!sendQueue_.isFull()) {
    sendQueue_.write(ssShared);
  } else {
    // Too many messages in queue
    LOG(WARNING) << "send_queue_ isFull!";
    return;
  }

  // Are we already writing?
  /*if (sendQueue_.size() > 1) {
    LOG(INFO) << "send_queue_.size() > 1";
    return;
  }*/

  if (!isOpen()) {
    LOG(WARNING) << "!ws_.is_open()";
    return;
  }

  if (!isSendBusy_ && !sendQueue_.isEmpty()) {
    isSendBusy_ = true;

    if (!sendQueue_.frontPtr() || !sendQueue_.frontPtr()->get()) {
      LOG(WARNING) << "ws: invalid sendQueue_.frontPtr()";
      return;
    }

    // std::shared_ptr<const std::string> dp = *(sendQueue_.frontPtr());
    std::shared_ptr<const std::string> dp;
    sendQueue_.read(dp);

    if (!dp || !dp.get()) {
      LOG(WARNING) << "invalid sendQueue_.front()) ";
      return;
    }

    // We are not currently writing, so send this immediately
    {
      ws_.text(
          true); // This controls whether or not outgoing message opcodes are set to binary or text.
      ws_.async_write(
          net::buffer(*dp),
          net::bind_executor(strand_, std::bind(&WsSession::on_write, shared_from_this(),
                                                std::placeholders::_1, std::placeholders::_2)));
    }
  }
}

bool WsSession::isExpired() const { return isExpired_; }

} // namespace net
} // namespace gloer

#include "net/ws/client/ClientSession.hpp" // IWYU pragma: associated
#include "net/ws/client/ClientSessionManager.hpp"
#include "algo/DispatchQueue.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManagerBase.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/SessionPair.hpp"
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
#include <webrtc/rtc_base/bind.h>
#include <webrtc/rtc_base/checks.h>

namespace gloer {
namespace net {
namespace ws {

// @note ::tcp::socket socket represents the local end of a connection between two peers
// NOTE: Following the std::move, the moved-from object is in the same state
// as if constructed using the basic_stream_socket(io_service&) constructor.
// boost.org/doc/libs/1_54_0/doc/html/boost_asio/reference/basic_stream_socket/basic_stream_socket/overload5.html
ClientSession::ClientSession(boost::asio::io_context& ioc,
    ::boost::asio::ssl::context& ctx,
    net::WSClientNetworkManager* nm,
    const ws::SessionGUID& id)
    : SessionPair(id)
      , ctx_(ctx)
      , ws_(boost::asio::make_strand(ioc))
      , nm_(nm)
      , isSendBusy_(false)
      , resolver_(boost::asio::make_strand(ioc))
{

  // TODO: stopped() == false
  // RTC_DCHECK(socket.get_executor().context().stopped() == false);

  RTC_DCHECK(ws_.is_open() == false);

  RTC_DCHECK(nm_ != nullptr);

  RTC_DCHECK_GT(static_cast<std::string>(id).length(), 0);

  RTC_DCHECK_LT(static_cast<std::string>(id).length(), MAX_ID_LEN);
}

ClientSession::~ClientSession() {
  // LOG(INFO) << "~ClientSession";
  const ws::SessionGUID wsConnId = getId(); // remember id before session deletion

  close();

  if (!onCloseCallback_) {
    LOG(WARNING) << "WRTCSession::onDataChannelMessage: Not set onMessageCallback_!";
    return;
  }

  onCloseCallback_(wsConnId);

  if (nm_ && nm_->getRunner().get()) {
    nm_->sessionManager().unregisterSession(wsConnId);
  }
}

void ClientSession::on_session_fail(beast::error_code ec, char const* what) {
  // Don't report these
  if (ec == ::boost::asio::error::operation_aborted
      || ec == ::websocket::error::closed) {
    return;
  }

  if (ec == beast::error::timeout) {
      LOG(INFO) << "|idle timeout when read"; //idle_timeout
      isExpired_ = true;
  }

  close();

  LOG(WARNING) << "ClientSession: " << what << " : " << ec.message();
  // const std::string wsGuid = boost::lexical_cast<std::string>(getId());
  ws::SessionGUID copyId = getId();
  nm_->sessionManager().unregisterSession(copyId);
}

void ClientSession::connectAsClient(const std::string& host, const std::string& port) {
  host_ = host;

  // Look up the domain name
  resolver_.async_resolve(
      host.c_str(),
      port.c_str(),
      beast::bind_front_handler(
          &ClientSession::onClientResolve,
          shared_from_this()));

  // Look up the domain name
  /*resolver_.async_resolve(
      host, port,
      boost::asio::bind_executor(ws_.get_executor(), std::bind(&ClientSession::onClientResolve, shared_from_this(),
                                                    std::placeholders::_1, std::placeholders::_2)));
  */
}

void ClientSession::onClientResolve(beast::error_code ec, tcp::resolver::results_type results) {
  if (ec)
    return on_session_fail(ec, "resolve");

  RTC_DCHECK(created_cb_);
  created_cb_("resolve");

#if 0
  // Make the connection on the IP address we get from a lookup
  ::boost::asio::async_connect(
      ws_.next_layer(), results.begin(), results.end(),
      /*boost::asio::bind_executor(ws_.get_executor(), std::bind(&ClientSession::onClientConnect, shared_from_this(),
                                                    std::placeholders::_1)));*/
      beast::bind_front_handler(
          &ClientSession::onClientConnect,
          shared_from_this()));
#endif // 0

  // Set the timeout for the operation
  beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(5));

  // Make the connection on the IP address we get from a lookup
  beast::get_lowest_layer(ws_).async_connect(
      results,
      beast::bind_front_handler(
          &ClientSession::onClientConnect,
          shared_from_this()));
}

void ClientSession::onClientConnect(beast::error_code ec, tcp::resolver::results_type::endpoint_type) {
  if (ec)
    return on_session_fail(ec, "connect");

  RTC_DCHECK(created_cb_);
  created_cb_("connect");

  // Turn off the timeout on the tcp_stream, because
  // the websocket stream has its own timeout system.
  beast::get_lowest_layer(ws_).expires_never();


  // TODO: SSL as in
  // github.com/vinniefalco/beast/blob/master/example/server-framework/main.cpp
  // Set options before performing the handshake.
  /**
   * Determines if outgoing message payloads are broken up into
   * multiple pieces.
   **/
  // ws_.auto_fragment(false);
  /**
   * Permessage-deflate allows messages to be compressed.
   **/
  ::websocket::permessage_deflate pmd;
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
  LOG(INFO) << "created ClientSession #" << static_cast<std::string>(id_);

  // Set a decorator to change the Server of the handshake
  ws_.set_option(websocket::stream_base::decorator(
      [](websocket::response_type& res)
      {
          res.set(http::field::user_agent,
              std::string(BOOST_BEAST_VERSION_STRING) +
                  " websocket-client-async");
      }));

  // Turn off the timeout on the tcp_stream, because
  // the websocket stream has its own timeout system.
  // beast::get_lowest_layer(ws_).expires_never();

  ///\see https://github.com/boostorg/beast/commit/f21358186ecad9744ed6c72618d4a4cfc36be5fb#diff-68c7d3776da215dd6d1b335448c77f3bR116
  ws_.set_option(websocket::stream_base::timeout{
                // Time limit on handshake, accept, and close operations
                std::chrono::seconds(30), // handshake_timeout
                // The time limit after which a connection is considered idle.
                std::chrono::seconds(300), // idle_timeout
                /*
                  If the idle interval is set, this setting affects the
                  behavior of the stream when no data is received for the
                  timeout interval as follows:
                  @li When `keep_alive_pings` is `true`, an idle ping will be
                  sent automatically. If another timeout interval elapses
                  with no received data then the connection will be closed.
                  An outstanding read operation must be pending, which will
                  complete immediately the error @ref beast::error::timeout.
                  @li When `keep_alive_pings` is `false`, the connection will be closed.
                  An outstanding read operation must be pending, which will
                  complete immediately the error @ref beast::error::timeout.
                */
                /// \note Prefer true on server and false on client.
                false} // keep_alive_pings.
  );

  // Perform the websocket handshake
  //auto host = beast::get_lowest_layer(ws_).local_endpoint().address().to_string();
  /*ws_.async_handshake(
      host, host,
      boost::asio::bind_executor(ws_.get_executor(), std::bind(&ClientSession::onClientHandshake,
                                                    shared_from_this(), std::placeholders::_1)));
  */
  ws_.async_handshake(
    host_, "/",
    //boost::asio::ssl::stream_base::client,
    beast::bind_front_handler(
        &ClientSession::onClientHandshake,
        shared_from_this()));
}

void ClientSession::onClientHandshake(beast::error_code ec) {
  if (ec)
    return on_session_fail(ec, "handshake");

  RTC_DCHECK(created_cb_);
  created_cb_("handshake");
}

void ClientSession::start_read() {
  LOG(INFO) << "WS session run as client";

  // Set the control callback. This will be called
  // on every incoming ping, pong, and close frame.
  /*ws_.control_callback(
      boost::asio::bind_executor(ws_.get_executor(),
        std::bind(&ClientSession::on_control_callback, this,
          std::placeholders::_1, std::placeholders::_2)));

  // Run the timer. The timer is operated
  // continuously, this simplifies the code.
  on_timer({});

  // Set the timer
  timer_.expires_after(std::chrono::seconds(WS_PING_FREQUENCY_SEC));*/

  setFullyCreated(true); // TODO

  // Read a message
  do_read();
}

bool ClientSession::waitForConnect(std::size_t maxWait_ms) const {
  auto end_time = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(maxWait_ms);
  auto current_time = std::chrono::high_resolution_clock::now();
  while (!isOpen() && (current_time < end_time)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    current_time = std::chrono::high_resolution_clock::now();
  }
  return isOpen();
}

size_t ClientSession::MAX_IN_MSG_SIZE_BYTE = 16 * 1024;
size_t ClientSession::MAX_OUT_MSG_SIZE_BYTE = 16 * 1024;

#if 0
void ClientSession::on_control_callback(::websocket::frame_type kind, beast::string_view payload) {
  // LOG(INFO) << "WS on_control_callback";
  boost::ignore_unused(kind, payload);

  // Note that there is activity
  onRemoteActivity();
}

// Called to indicate activity from the remote peer
void ClientSession::onRemoteActivity() {
  // Note that the connection is alive
  pingState_ = PING_STATE::ALIVE;

  // Set the timer
  timer_.expires_after(std::chrono::seconds(WS_PING_FREQUENCY_SEC));
}

// Called after a ping is sent.
void ClientSession::on_ping(beast::error_code ec) {
  // Happens when the timer closes the socket
  if (ec == ::boost::asio::error::operation_aborted) {
    LOG(WARNING) << "ClientSession on_ping ec:" << ec.message();
    return;
  }

  if (ec) {
    LOG(WARNING) << "ClientSession on_ping ec:" << ec.message();
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
void ClientSession::on_timer(beast::error_code ec) {
  // LOG(INFO) << "ClientSession::on_timer";

  if (ec && ec != ::boost::asio::error::operation_aborted) {
    LOG(WARNING) << "ClientSession on_timer ec:" << ec.message();
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
      ws_.async_ping(
          {}, ::boost::asio::bind_executor(ws_.get_executor(), std::bind(&ClientSession::on_ping, shared_from_this(),
                                                      std::placeholders::_1)));
    } else {
      // The timer expired while trying to handshake,
      // or we sent a ping and it never completed or
      // we never got back a control frame, so close.

      // Closing the socket cancels all outstanding operations. They
      // will complete with ::boost::asio::error::operation_aborted

      // LOG(INFO) << "Closing expired WS session";

      // LOG(INFO) << "on_timer: total ws sessions: " << nm_->sessionManager().getSessionsCount();
      // ws_.next_layer().shutdown(::tcp::socket::shutdown_both, ec);
      // ws_.next_layer().close(ec);

      close();

      ws::SessionGUID copyId = getId();
      nm_->sessionManager().unregisterSession(copyId);
      isExpired_ = true;
      return;
    }
  }

  // Wait on the timer
  timer_.async_wait(::boost::asio::bind_executor(
      ws_.get_executor(), std::bind(&ClientSession::on_timer, shared_from_this(), std::placeholders::_1)));
}
#endif // 0

void ClientSession::close() {
  if (!ws_.is_open()) {
    // LOG(WARNING) << "Close error: Tried to close already closed webSocket, ignoring...";
    return;
  }
  /*boost::system::error_code errorCode;
  ws_.close(boost::beast::websocket::close_reason(boost::beast::websocket::close_code::normal),
            errorCode);
  if (errorCode) {
    LOG(WARNING) << "ClientSession: Close error: " << errorCode.message();
  }*/

  // Close the WebSocket connection
  ws_.async_close(websocket::close_code::normal,
      beast::bind_front_handler(
          &ClientSession::on_close,
          shared_from_this()));
}

void ClientSession::on_close(beast::error_code ec)
{
  if(ec)
      return on_session_fail(ec, "close");

  // If we get here then the connection is closed gracefully

  // The make_printable() function helps print a ConstBufferSequence
  // LOG(INFO) << beast::make_printable(buffer_.data());
}

void ClientSession::do_read() {
  // LOG(INFO) << "WS session do_read";

  // Set the timer
  // timer_.expires_after(std::chrono::seconds(WS_PING_FREQUENCY_SEC));

  /*if (!isOpen()) {
    LOG(WARNING) << "!ws_.is_open()";
    return;
  }*/

  // Read a message into our buffer
  /*ws_.async_read(
      recievedBuffer_,
      ::boost::asio::bind_executor(ws_.get_executor(), std::bind(&ClientSession::on_read, shared_from_this(),
                                              std::placeholders::_1, std::placeholders::_2)));
  */

  // Read a message into our buffer
  ws_.async_read(
      recievedBuffer_,
      beast::bind_front_handler(
          &ClientSession::on_read,
          shared_from_this()));
}

void ClientSession::on_read(beast::error_code ec, std::size_t bytes_transferred) {
  // LOG(INFO) << "WS session on_read";

  boost::ignore_unused(bytes_transferred);

  // Happens when the timer closes the socket
  if (ec == ::boost::asio::error::operation_aborted) {
    LOG(WARNING) << "ClientSession on_read: ::boost::asio::error::operation_aborted";
    return;
  }

  // This indicates that the session was closed
  if (ec == ::websocket::error::closed) {
    LOG(WARNING) << "ClientSession on_read ec:" << ec.message();
    return; // on_session_fail(ec, "write");
  }

  if (ec)
    on_session_fail(ec, "read");

  // Note that there is activity
  // onRemoteActivity();

  // LOG(INFO) << "buffer: " <<
  // beast::buffers_to_string(recieved_buffer_.data());
  // send(beast::buffers_to_string(recieved_buffer_.data())); // ??????

  if (!recievedBuffer_.size()) {
    // may be empty if connection reset by peer
    // LOG(WARNING) << "ClientSession::on_read: empty messageBuffer";
    return;
  }

  if (recievedBuffer_.size() > MAX_IN_MSG_SIZE_BYTE) {
    LOG(WARNING) << "ClientSession::on_read: Too big messageBuffer of size " << recievedBuffer_.size();
    return;
  }

  // add incoming message callback into queue
  // TODO: use protobuf
  /*auto sharedBuffer =
      std::make_shared<std::string>(beast::buffers_to_string(recievedBuffer_.data()));*/
  const std::string data = beast::buffers_to_string(recievedBuffer_.data());

  // handleIncomingJSON(sharedBuffer);

  // handleIncomingJSON(data);
  if (!onMessageCallback_) {
    LOG(WARNING) << "ClientSession::on_read: Not set onMessageCallback_!";
    return;
  }

  LOG(WARNING) << "ClientSession on_read: " << data;

  onMessageCallback_(getId(), data);

  // Clear the buffer
  recievedBuffer_.consume(recievedBuffer_.size());

  // Do another read
  do_read();
}

/*std::shared_ptr<algo::DispatchQueue> ClientSession::getWRTCQueue() const {
  return getRunner()->getWRTCQueue();
}*/

//#if ENABLE_WRTC
void ClientSession::pairToWRTCSession(std::shared_ptr<wrtc::WRTCSession> WRTCSession) {
  rtc::CritScope lock(&wrtcSessMutex_);
  LOG(INFO) << "pairToWRTCSessionn...";
  // rtc::CritScope lock(&wrtcSessMutex_);
  if (!WRTCSession) {
    LOG(WARNING) << "pairToWRTCSession: Invalid WRTCSession";
    return;
  }
  wrtcSession_ = WRTCSession;
}

bool ClientSession::hasPairedWRTCSession() {
  rtc::CritScope lock(&wrtcSessMutex_);
  return wrtcSession_.lock().get() != nullptr;
}

std::weak_ptr<wrtc::WRTCSession> ClientSession::getWRTCSession() const {
  rtc::CritScope lock(&wrtcSessMutex_);
  if (!wrtcSession_.lock()) {
    LOG(WARNING) << "getWRTCSession: Invalid wrtcSession_";
    return wrtcSession_;
  }
  return wrtcSession_;
}
//#endif // ENABLE_WRTC

bool ClientSession::isOpen() const { return ws_.is_open(); }

void ClientSession::on_write(beast::error_code ec, std::size_t bytes_transferred) {
  // LOG(INFO) << "WS session on_write";
  boost::ignore_unused(bytes_transferred);

  // Happens when the timer closes the socket
  if (ec == ::boost::asio::error::operation_aborted) {
    LOG(WARNING) << "ClientSession on_write: ::boost::asio::error::operation_aborted: " << ec.message();
    return;
  }

  if (ec) {
    LOG(WARNING) << "ClientSession on_write: ec";
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
    ws_.text(true); // TODO: ws().text(derived().ws().got_text());
    ws_.async_write(
        ::boost::asio::buffer(*dp),
        /*::boost::asio::bind_executor(strand_, std::bind(&ClientSession::on_write, shared_from_this(),
                                                std::placeholders::_1, std::placeholders::_2)));
        */
        beast::bind_front_handler(
                        &ClientSession::on_write,
                        shared_from_this()));
  } else {
    LOG(INFO) << "write send_queue_.empty()";
    isSendBusy_ = false;
  }
}

/**
 * @brief starts async writing to client
 *
 * @param message message passed to client
 */
void ClientSession::send(const std::string& ss) {
  // LOG(WARNING) << "ClientSession::send:" << ss;
  std::shared_ptr<const std::string> ssShared =
      std::make_shared<const std::string>(ss); // TODO: std::move

  if (!ssShared || !ssShared.get() || ssShared->empty()) {
    LOG(WARNING) << "ClientSession::send: empty messageBuffer";
    return;
  }

  if (ssShared->size() > MAX_OUT_MSG_SIZE_BYTE) {
    LOG(WARNING) << "ClientSession::send: Too big messageBuffer of size " << ssShared->size();
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
      // This controls whether or not outgoing message opcodes are set to binary or text.
      ws_.text(true); // TODO: ws().text(derived().ws().got_text());
      ws_.async_write(
          ::boost::asio::buffer(*dp),
          /*::boost::asio::bind_executor(strand_, std::bind(&ClientSession::on_write, shared_from_this(),
                                                  std::placeholders::_1, std::placeholders::_2)));
          */
          beast::bind_front_handler(
                          &ClientSession::on_write,
                          shared_from_this()));
    }
  }
}

bool ClientSession::isExpired() const { return isExpired_; }

} // namespace ws
} // namespace net
} // namespace gloer

#include "net/WsSession.hpp"
#include "net/SessionManager.hpp"

/*#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.*/

namespace utils {
namespace net {

void WsSession::on_session_fail(beast::error_code ec, char const* what) {
  std::cerr << "WsSession" << what << ": " << ec.message() << "\n";
  // const std::string wsGuid = boost::lexical_cast<std::string>(getId());
  sm_->unregisterSession(getId());
  // TODO: check
  /*ws_.next_layer().shutdown(tcp::socket::shutdown_both, ec);
  ws_.next_layer().close(ec);*/
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
  /*on_timer({});

  // Set the timer
  timer_.expires_after(std::chrono::seconds(15));*/

  // Accept the websocket handshake
  // Start reading and responding to a WebSocket HTTP Upgrade request.
  ws_.async_accept(net::bind_executor(
      strand_, std::bind(&WsSession::on_accept, shared_from_this(),
                         std::placeholders::_1)));
}

void WsSession::on_control_callback(websocket::frame_type kind,
                                    beast::string_view payload) {
  boost::ignore_unused(kind, payload);

  // Note that there is activity
  activity();
}

// Called to indicate activity from the remote peer
void WsSession::activity() {
  // Note that the connection is alive
  ping_state_ = 0;

  // Set the timer
  // timer_.expires_after(std::chrono::seconds(15));
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
  if (ec == net::error::operation_aborted)
    return;

  if (ec)
    return on_session_fail(ec, "ping");

  // Note that the ping was sent.
  if (ping_state_ == 1) {
    ping_state_ = 2;
  } else {
    // ping_state_ could have been set to 0
    // if an incoming control frame was received
    // at exactly the same time we sent a ping.
    BOOST_ASSERT(ping_state_ == 0);
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
    if (ws_.is_open() && ping_state_ == 0) {
      // Note that we are sending a ping
      ping_state_ = 1;

      // Set the timer
      timer_.expires_after(std::chrono::seconds(15));

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
  // std::cout << "buffer: " << beast::buffers_to_string(buffer_.data()) <<
  // "\n";
  // Read a message into our buffer
  ws_.async_read(buffer_,
                 net::bind_executor(strand_, std::bind(&WsSession::on_read,
                                                       shared_from_this(),
                                                       std::placeholders::_1,
                                                       std::placeholders::_2)));
}

void WsSession::on_read(beast::error_code ec, std::size_t bytes_transferred) {
  std::cout << "WS session on_read\n";

  // std::cout << "buffer: " << beast::buffers_to_string(buffer_.data()) <<
  // "\n";
  boost::ignore_unused(bytes_transferred);

  // Happens when the timer closes the socket
  if (ec == net::error::operation_aborted)
    return;

  // This indicates that the session was closed
  if (ec == websocket::error::closed)
    return;

  if (ec)
    on_session_fail(ec, "read");

  // Note that there is activity
  activity();

  // send(beast::buffers_to_string(buffer_.data())); // ??????

  // Clear the buffer
  buffer_.consume(buffer_.size());

  // Do another read
  do_read();
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

  /*// Echo the message
  bool isTextMessage = ws_.got_text();
  ws_.text(isTextMessage); // whether or not outgoing message opcodes are set
                           // to binary or text*/

  if (!queue_.empty()) {
    std::cout << "write buffer: " << *queue_.front() << "\n";

    /*// Are we already writing?
    if (queue_.size() > 1) {
      std::cerr << "queue_.size() > 1\n";
      return;
    }*/
    /*if (!busy_) {
      busy_ = true;*/
    ws_.async_write(
        // was buffer_.data(),
        net::buffer(*queue_.front()),
        net::bind_executor(
            strand_, std::bind(&WsSession::on_write, shared_from_this(),
                               std::placeholders::_1, std::placeholders::_2)));
    //}
  } else {
    std::cout << "write queue_.empty()\n";
    busy_ = false;
  }
}

/*void WsSession::send(const std::string& ss) {
  send(std::make_shared<std::string>(ss));
}*/

/**
 * @brief starts async writing to client
 *
 * @param message message passed to client
 */
void WsSession::send(const std::string ss) {
  auto const ssShared = std::make_shared<std::string const>(std::move(ss));

  // Always add to queue
  queue_.push_back(ssShared);

  // Are we already writing?
  if (queue_.size() > 1) {
    std::cerr << "queue_.size() > 1\n";
    return;
  }

  if (!busy_) {
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

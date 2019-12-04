#include "net/ws/WsListener.hpp" // IWYU pragma: associated
#include "algo/StringUtils.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include "net/ws/WsServer.hpp"
#include "net/ws/WsSession.hpp"
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <webrtc/rtc_base/bind.h>
#include <webrtc/rtc_base/checks.h>
#include <webrtc/rtc_base/criticalsection.h>
#include <webrtc/rtc_base/sequenced_task_checker.h>

namespace gloer {
namespace net {
namespace ws {

namespace {

// TODO: prevent collision? respond ERROR to client if collided?
static std::string nextWsSessionId() { return gloer::algo::genGuid(); }

} // namespace

WsListener::WsListener(
  ::boost::asio::io_context& ioc,
  ::boost::asio::ssl::context& ctx,
  const ::boost::asio::ip::tcp::endpoint& endpoint,
  std::shared_ptr<std::string const> doc_root, NetworkManager* nm)
    : acceptor_(ioc)
      //, socket_(ioc)
      , ioc_(ioc)
      , ctx_(ctx)
      , doc_root_(doc_root)
      , nm_(nm)
      , endpoint_(endpoint)
      // , strand_(boost::asio::make_strand(ioc.get_executor()))
{
  configureAcceptor();
}

// Report a failure
void WsListener::on_WsListener_fail(beast::error_code ec, char const* what) {

  // NOTE: If you got on_WsListener_fail: accept: Too many open files
  // set ulimit -n 4096, see stackoverflow.com/a/8583083/10904212
  // Restart the accept operation if we got the connection_aborted error
  // and the enable_connection_aborted socket option is not set.
  if (ec == ::boost::asio::error::connection_aborted /*&& !enable_connection_aborted_*/) {
    LOG(WARNING) << "on_WsListener_fail ::boost::asio::error::connection_aborted";
    // TODO
    // https://github.com/nesbox/boost_1_68-android/blob/master/armeabi-v7a/include/boost-1_68/boost/asio/detail/win_iocp_socket_accept_op.hpp#L222
    /*o->reset();
    o->socket_service_.restart_accept_op(o->socket_, o->new_socket_, o->protocol_.family(),
                                         o->protocol_.type(), o->protocol_.protocol(),
                                         o->output_buffer(), o->address_length(), o);
    p.v = p.p = 0;
    return;*/
  }

  // Don't report these
  if (ec == ::boost::asio::error::operation_aborted || ec == ::websocket::error::closed)
    return;

  LOG(WARNING) << "on_WsListener_fail: " << what << ": " << ec.message();
}

void WsListener::configureAcceptor() {
  beast::error_code ec;

  LOG(INFO) << "configuring acceptor for " << endpoint_.address().to_string();

  // Open the acceptor
  acceptor_.open(endpoint_.protocol(), ec);
  if (ec) {
    on_WsListener_fail(ec, "open");
    return;
  }
  RTC_DCHECK(isAccepting() == true);

  // @see boost.org/doc/libs/1_61_0/doc/html/boost_asio/reference/basic_socket.html
  {
    // Allow address reuse
    // NOTE: In windows, the option tcp::acceptor::reuse_address
    // is equivalent to calling setsockopt and specifying SO_REUSEADDR.
    // This specifically allows multiple sockets to be bound
    // to an address even if it is in use.
    // @see stackoverflow.com/a/7195105/10904212
    acceptor_.set_option(::boost::asio::socket_base::reuse_address(true), ec);
    if (ec) {
      on_WsListener_fail(ec, "set_option");
      return;
    }
    /*if (enable_connection_aborted_) {
      acceptor_.set_option(::boost::asio::socket_base::enable_connection_aborted(true), ec);
      if (ec) {
        on_WsListener_fail(ec, "set_option");
        return;
      }
    }
    acceptor_.set_option(::boost::asio::socket_base::keep_alive(true), ec);
    if (ec) {
      on_WsListener_fail(ec, "set_option");
      return;
    }
    acceptor_.set_option(::boost::asio::socket_base::send_buffer_size(8192), ec);
    if (ec) {
      on_WsListener_fail(ec, "set_option");
      return;
    }
    acceptor_.set_option(::boost::asio::socket_base::receive_buffer_size(8192), ec);
    if (ec) {
      on_WsListener_fail(ec, "set_option");
      return;
    }*/
  }

  // Bind to the server address
  acceptor_.bind(endpoint_, ec);
  if (ec) {
    on_WsListener_fail(ec, "bind");
    return;
  }

  // Start listening for connections
  if (max_listen_connections_ < 0) {
    acceptor_.listen(::boost::asio::socket_base::max_listen_connections, ec);
    max_listen_connections_ = ::boost::asio::socket_base::max_listen_connections;
  } else {
    acceptor_.listen(max_listen_connections_, ec);
  }
  if (ec) {
    on_WsListener_fail(ec, "listen");
    return;
  }
  LOG(INFO) << "set WS max_listen_connections to " << max_listen_connections_;
}

// Start accepting incoming connections
/*void WsListener::setMode(WS_LISTEN_MODE mode) {
  if (!mode_ || !mode_.get()) {
    mode_ = std::make_unique<WS_LISTEN_MODE>(mode);
  } else {
    mode_->_value = mode;
  }
}*/

// Don`t accept incoming connections
void WsListener::run(/*WS_LISTEN_MODE mode*/) {
  //setMode(mode);

  LOG(INFO) << "WsListener run";

  RTC_DCHECK(isAccepting() == true && needClose_ == false);
  if (!isAccepting() || needClose_) {
    LOG(INFO) << "WsListener::run: not accepting";
    return; // stop on_accept recursion
  }

  do_accept();
}

void WsListener::do_accept() {
  LOG(INFO) << "WS do_accept";

  if (needClose_) {
    LOG(WARNING) << "WsListener::do_accept: need close";
    return; // stop on_accept recursion
  }
  /**
   * I/O objects such as sockets and streams are not thread-safe. For efficiency, networking adopts
   * a model of using threads without explicit locking by requiring all access to I/O objects to be
   * performed within a strand.
   */
  acceptor_.async_accept(
      // The new connection gets its own strand
      ::boost::asio::make_strand(ioc_),
      beast::bind_front_handler(
          &WsListener::on_accept,
          shared_from_this()));
}

// Stop accepting incoming connections
void WsListener::stop() {
  RTC_DCHECK(isAccepting() == true && needClose_ == false);

  try {
    beast::error_code ec;
    // For portable behaviour with respect to graceful closure of a connected
    // socket, call shutdown() before closing the socket.
    /*if (socket_.is_open()) {
      socket_.shutdown(::boost::asio::ip::tcp::socket::shutdown_both, ec);
      LOG(INFO) << "shutdown socket...";
      if (ec) {
        on_WsListener_fail(ec, "socket_shutdown");
      }
      socket_.close(ec);
      LOG(INFO) << "close socket...";
      if (ec) {
        on_WsListener_fail(ec, "socket_close");
      }
    }*/

    // TODO: loop over open sessions and close them all

    if (acceptor_.is_open()) {
      LOG(INFO) << "close acceptor...";
      acceptor_.close(ec);
      if (ec) {
        on_WsListener_fail(ec, "acceptor_close");
      }
    }
    needClose_ = true;
  } catch (const boost::system::system_error& ex) {
    LOG(WARNING) << "WsListener::stop: exception: " << ex.what();
  }

#if 0
  /**
   * @see github.com/boostorg/beast/issues/940
   * Calls to cancel() will always fail with ::boost::asio::error::operation_not_supported when run on
   * Windows XP, Windows Server 2003, and earlier versions of Windows, unless
   * BOOST_ASIO_ENABLE_CANCELIO is defined. However, the CancelIo function has two issues
   * boost.org/doc/libs/1_47_0/doc/html/boost_asio/reference/basic_stream_socket/cancel/overload1.html
   **/
  ::boost::asio::post(socket_.get_executor(), ::boost::asio::bind_executor(strand_, [&]() {
                try {
                  beast::error_code ec;
                  // For portable behaviour with respect to graceful closure of a connected
                  // socket, call shutdown() before closing the socket.
                  if (socket_.is_open()) {
                    socket_.shutdown(::boost::asio::ip::tcp::socket::shutdown_both, ec);
                    LOG(INFO) << "shutdown socket...";
                    if (ec) {
                      on_WsListener_fail(ec, "socket_shutdown");
                    }
                    socket_.close(ec);
                    LOG(INFO) << "close socket...";
                    if (ec) {
                      on_WsListener_fail(ec, "socket_close");
                    }
                  }
                  if (acceptor_.is_open()) {
                    LOG(INFO) << "close acceptor...";
                    acceptor_.close(ec);
                    if (ec) {
                      on_WsListener_fail(ec, "acceptor_close");
                    }
                  }
                  needClose_ = true;
                } catch (const boost::system::system_error& ex) {
                  LOG(WARNING) << "WsListener::stop: exception: " << ex.what();
                }
              }));
#endif // 0
}

#if 0
std::shared_ptr<WsSession> WsListener::addClientSession(
  const std::string& newSessId)
{
  if (mode_->_value == WS_LISTEN_MODE::SERVER) {
    LOG(INFO) << "WsListener::addClientSession: client session not compatible with "
                 "WS_LISTEN_MODE::SERVER";
    return nullptr;
  }

  if (needClose_) {
    LOG(INFO) << "WsListener::addClientSession: need close";
    return nullptr;
  }

  // NOTE: Following the move, the moved-from object is in the same state
  // as if constructed using the basic_stream_socket(io_service&) constructor.
  // boost.org/doc/libs/1_54_0/doc/html/boost_asio/reference/basic_stream_socket/basic_stream_socket/overload5.html
  auto newWsSession
    = std::make_shared<WsSession>(
        std::move(socket_), ctx_, nm_, newSessId);
  nm_->getWS_SM().addSession(newSessId, newWsSession);
  return newWsSession;
}
#endif // 0

/**
 * @brief handles new connections and starts sessions
 */
void WsListener::on_accept(beast::error_code ec, ::boost::asio::ip::tcp::socket socket) {
  LOG(INFO) << "WS on_accept";

  // RTC_DCHECK(isAccepting() == true && socket.is_open() && needClose_ == false);
  if (!isAccepting() || !socket.is_open() || needClose_) {
    LOG(WARNING) << "WsListener::on_accept: not accepting";
    return; // stop on_accept recursion
  }

  if (ec) {
    on_WsListener_fail(ec, "accept");
  } else {

    // RTC_DCHECK(isAccepting() == true && socket.is_open() && needClose_ == false);
    if (!isAccepting() || !socket.is_open() || needClose_) {
      LOG(WARNING) << "WsListener::on_accept: not accepting";
      return; // stop on_accept recursion
    }

    const bool canCreateSessionsByRequest = true;
        //mode_->_value == WS_LISTEN_MODE::SERVER || mode_->_value == WS_LISTEN_MODE::BOTH;

    const auto connectionsCount = nm_->getWS_SM().getSessionsCount();

    // Create the session and run it
    const auto newSessId = nextWsSessionId();

    // NOTE: check will be executed regardless of compilation mode.
    RTC_CHECK(max_listen_connections_ > 1);
    RTC_CHECK(max_sessions_count > 1);

    if (connectionsCount >= (max_listen_connections_ - 1) ||
        connectionsCount >= (max_sessions_count - 1)) {
      LOG(WARNING) << "Reached WS max_listen_connections = " << max_listen_connections_;
      LOG(WARNING) << "Or reached WS max_sessions_count = " << max_sessions_count;
      LOG(WARNING) << "WS Sessions Count = " << connectionsCount;
      auto newWsSession
        = std::make_unique<WsSession>(
            std::move(socket),
            ctx_,
            nm_,
            newSessId);
      newWsSession->send("REACHED_MAX_SESSION_COUNT");
      newWsSession->close();
    } else if (canCreateSessionsByRequest) {

      // NOTE: Following the std::move, the moved-from object is in the same state as if
      // constructed using the basic_stream_socket(io_service&) constructor.
      // boost.org/doc/libs/1_54_0/doc/html/boost_asio/reference/basic_stream_socket/basic_stream_socket/overload5.html
      auto newWsSession = std::make_shared<WsSession>(
        std::move(socket), ctx_, nm_, newSessId);
      nm_->getWS_SM().addSession(newSessId, newWsSession);

      if (!nm_->getWS_SM().onNewSessCallback_) {
        LOG(WARNING) << "WRTC: Not set onNewSessCallback_!";
        return;
      }

      nm_->getWS_SM().onNewSessCallback_(newWsSession);

      newWsSession->runAsServer();
    }
  }

  // Accept another connection
  do_accept();
} // namespace ws

} // namespace ws
} // namespace net
} // namespace gloer

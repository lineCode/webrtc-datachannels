#include "net/http/server/ServerSession.hpp" // IWYU pragma: associated
#include "net/ws/server/ServerSession.hpp"
#include "net/http/server/ServerSessionManager.hpp"
#include "algo/DispatchQueue.hpp"
#include "algo/StringUtils.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManagerBase.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/wrtc/WRTCSession.hpp"
#include "net/http/server/ServerConnectionManager.hpp"
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
#include <net/http/SessionGUID.hpp>
#include "net/http/server/HTTPServerNetworkManager.hpp"
#include "net/http/server/ServerConnectionManager.hpp"
#include "net/http/server/ServerSessionManager.hpp"
#include "net/http/server/ServerInputCallbacks.hpp"
#include "net/ws/client/WSClientNetworkManager.hpp"
#include "net/ws/server/WSServerNetworkManager.hpp"
#include "net/ws/server/ServerSessionManager.hpp"
#include "net/ws/server/ServerSessionManager.hpp"
#include "net/ws/server/ServerInputCallbacks.hpp"

/**
 * The amount of time to wait in seconds, before sending a websocket 'ping'
 * message. Ping messages are used to determine if the remote end of the
 * connection is no longer available.
 **/
//constexpr unsigned long WS_PING_FREQUENCY_SEC = 10;

/*namespace {

BETTER_ENUM(PING_STATE, uint32_t, ALIVE, SENDING, SENT, TOTAL)

}*/

namespace {

// TODO: prevent collision? respond ERROR to client if collided?
static gloer::net::ws::SessionGUID nextWsSessionId() {
  return gloer::net::ws::SessionGUID(gloer::algo::genGuid());
}

// Return a reasonable mime type based on the extension of a file.
beast::string_view
mime_type(beast::string_view path)
{
    using beast::iequals;
    auto const ext = [&path]
    {
        auto const pos = path.rfind(".");
        if(pos == beast::string_view::npos)
            return beast::string_view{};
        return path.substr(pos);
    }();
    if(iequals(ext, ".htm"))  return "text/html";
    if(iequals(ext, ".html")) return "text/html";
    if(iequals(ext, ".php"))  return "text/html";
    if(iequals(ext, ".css"))  return "text/css";
    if(iequals(ext, ".txt"))  return "text/plain";
    if(iequals(ext, ".js"))   return "application/javascript";
    if(iequals(ext, ".json")) return "application/json";
    if(iequals(ext, ".xml"))  return "application/xml";
    if(iequals(ext, ".swf"))  return "application/x-shockwave-flash";
    if(iequals(ext, ".flv"))  return "video/x-flv";
    if(iequals(ext, ".png"))  return "image/png";
    if(iequals(ext, ".jpe"))  return "image/jpeg";
    if(iequals(ext, ".jpeg")) return "image/jpeg";
    if(iequals(ext, ".jpg"))  return "image/jpeg";
    if(iequals(ext, ".gif"))  return "image/gif";
    if(iequals(ext, ".bmp"))  return "image/bmp";
    if(iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
    if(iequals(ext, ".tiff")) return "image/tiff";
    if(iequals(ext, ".tif"))  return "image/tiff";
    if(iequals(ext, ".svg"))  return "image/svg+xml";
    if(iequals(ext, ".svgz")) return "image/svg+xml";
    return "application/text";
}

// Append an HTTP rel-path to a local filesystem path.
// The returned path is normalized for the platform.
std::string
path_cat(
    beast::string_view base,
    beast::string_view path)
{
    if(base.empty())
        return std::string(path);
    std::string result(base);
#ifdef BOOST_MSVC
    char constexpr path_separator = '\\';
    if(result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
    for(auto& c : result)
        if(c == '/')
            c = path_separator;
#else
    char constexpr path_separator = '/';
    if(result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
#endif
    return result;
}

// This function produces an HTTP response for the given
// request. The type of the response object depends on the
// contents of the request, so the interface requires the
// caller to pass a generic lambda for receiving the response.
template<
    class Body, class Allocator,
    class Send>
void
handle_request(
    beast::string_view doc_root,
    beast::http::request<Body, beast::http::basic_fields<Allocator>>&& req,
    Send&& send)
{
    // Returns a bad request response
    auto const bad_request =
    [&req](beast::string_view why)
    {
        beast::http::response<beast::http::string_body> res{beast::http::status::bad_request, req.version()};
        res.set(beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(beast::http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
    };

    // Returns a not found response
    auto const not_found =
    [&req](beast::string_view target)
    {
        beast::http::response<beast::http::string_body> res{beast::http::status::not_found, req.version()};
        res.set(beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(beast::http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "The resource '" + std::string(target) + "' was not found.";
        res.prepare_payload();
        return res;
    };

    // Returns a server error response
    auto const server_error =
    [&req](beast::string_view what)
    {
        beast::http::response<beast::http::string_body> res{beast::http::status::internal_server_error, req.version()};
        res.set(beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(beast::http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + std::string(what) + "'";
        res.prepare_payload();
        return res;
    };

    // Make sure we can handle the method
    if( req.method() != beast::http::verb::get &&
        req.method() != beast::http::verb::head)
        return send(bad_request("Unknown HTTP-method"));

    // Request path must be absolute and not contain "..".
    if( req.target().empty() ||
        req.target()[0] != '/' ||
        req.target().find("..") != beast::string_view::npos)
        return send(bad_request("Illegal request-target"));

    // Build the path to the requested file
    std::string path = path_cat(doc_root, req.target());
    if(req.target().back() == '/')
        path.append("index.html");

    // Attempt to open the file
    beast::error_code ec;
    beast::http::file_body::value_type body;
    body.open(path.c_str(), beast::file_mode::scan, ec);

    // Handle the case where the file doesn't exist
    if(ec == boost::system::errc::no_such_file_or_directory)
        return send(not_found(req.target()));

    // Handle an unknown error
    if(ec)
        return send(server_error(ec.message()));

    // Cache the size since we need it after the move
    auto const size = body.size();

    // Respond to HEAD request
    if(req.method() == beast::http::verb::head)
    {
        beast::http::response<beast::http::empty_body> res{beast::http::status::ok, req.version()};
        res.set(beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(beast::http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return send(std::move(res));
    }

    // Respond to GET request
    beast::http::response<beast::http::file_body> res{
        std::piecewise_construct,
        std::make_tuple(std::move(body)),
        std::make_tuple(beast::http::status::ok, req.version())};
    res.set(beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(beast::http::field::content_type, mime_type(path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return send(std::move(res));
}

} // namespace

namespace gloer {
namespace net {
namespace http {


// @note ::tcp::socket socket represents the local end of a connection between two peers
// NOTE: Following the std::move, the moved-from object is in the same state
// as if constructed using the basic_stream_socket(io_service&) constructor.
// boost.org/doc/libs/1_54_0/doc/html/boost_asio/reference/basic_stream_socket/basic_stream_socket/overload5.html
ServerSession::ServerSession(boost::asio::ip::tcp::socket&& socket,
  ::boost::asio::ssl::context& ctx, net::http::HTTPServerNetworkManager* nm,
  net::WSServerNetworkManager* ws_nm, const http::SessionGUID& id)
    : SessionBase<http::SessionGUID>(id)
      //,
      , ctx_(ctx)
      , stream_(std::move(socket))
      , nm_(nm)
      , ws_nm_(ws_nm)
{

  // TODO: stopped() == false
  // RTC_DCHECK(socket.get_executor().context().stopped() == false);

  RTC_DCHECK(nm_ != nullptr);

  RTC_DCHECK_GT(static_cast<std::string>(id).length(), 0);

  RTC_DCHECK_LT(static_cast<std::string>(id).length(), MAX_ID_LEN);

#if 0
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
  LOG(INFO) << "created WsSession #" << static_cast<std::string>(id_);

  // Set a decorator to change the Server of the handshake
  ws_.set_option(websocket::stream_base::decorator(
      [](websocket::response_type& res)
      {
          res.set(http::field::server,
              std::string(BOOST_BEAST_VERSION_STRING) +
                  " websocket-server-async");
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
                true} // keep_alive_pings.
  );
#endif // 0
}

ServerSession::~ServerSession() {
  const http::SessionGUID httpConnId = getId(); // remember id before session deletion

  LOG(INFO) << "destroyed http session with id = " << static_cast<std::string>(httpConnId);

  if (nm_ && nm_->getRunner().get()) {
    nm_->sessionManager().unregisterSession(httpConnId);
  }

  RTC_DCHECK(!nm_->sessionManager().getSessById(httpConnId));

#if 0
  close();

  if (!onCloseCallback_) {
    LOG(WARNING) << "HTTP Session::onDataChannelMessage: Not set onMessageCallback_!";
    return;
  }

  onCloseCallback_(httpConnId);

#endif // 0
}

void ServerSession::on_session_fail(beast::error_code ec, char const* what) {
  LOG(WARNING) << "HTTP ServerSession: " << what << " : " << ec.message();

  // Don't report these
  /*if (ec == ::boost::asio::error::operation_aborted
      || ec == ::websocket::error::closed) {
    return;
  }

  if (ec == beast::error::timeout) {
      LOG(INFO) << "|idle timeout when read"; //idle_timeout
      isExpired_ = true;
  }*/
  // const std::string wsGuid = boost::lexical_cast<std::string>(getId());
  const http::SessionGUID copyId = getId();

  /// \note must free shared pointer and close connection in destructor
  nm_->sessionManager().unregisterSession(copyId);

  RTC_DCHECK(!nm_->sessionManager().getSessById(copyId));

  // Don't report on canceled operations
  if(ec == ::boost::asio::error::operation_aborted)
      return;

  LOG(WARNING) << "HTTP ServerSession on_session_fail: " << what << " : " << ec.message();
}

size_t ServerSession::MAX_IN_MSG_SIZE_BYTE = 16 * 1024;
size_t ServerSession::MAX_OUT_MSG_SIZE_BYTE = 16 * 1024;

// Start the asynchronous operation
void ServerSession::do_read() {
  LOG(INFO) << "WS session run";

  // Construct a new parser for each message
  parser_.emplace();

  // Apply a reasonable limit to the allowed size
  // of the body in bytes to prevent abuse.
  parser_->body_limit(10000);

  // Set the timeout.
  stream_.expires_after(std::chrono::seconds(30));

  // Read a request
  beast::http::async_read(
      stream_,
      buffer_,
      parser_->get(),
      beast::bind_front_handler(
          &http::ServerSession::on_read,
          shared_from_this()));
}

#if 0
void ServerSession::close() {
#if 0
  if (!ws_.is_open()) {
    // LOG(WARNING) << "Close error: Tried to close already closed webSocket, ignoring...";
    //beast::error_code ec(beast::error::timeout);
    //on_session_fail(ec, "timeout");
    //ws::SessionGUID copyId = getId();
    //nm_->sessionManager().(copyId);
    const ws::SessionGUID copyId = getId();
    RTC_DCHECK(!nm_->sessionManager().getSessById(copyId));
    return;
  }
  /*boost::system::error_code errorCode;
  ws_.close(boost::beast::websocket::close_reason(boost::beast::websocket::close_code::normal),
            errorCode);
  if (errorCode) {
    LOG(WARNING) << "WsSession: Close error: " << errorCode.message();
  }*/

  // Close the WebSocket connection
  ws_.async_close(websocket::close_code::normal,
      beast::bind_front_handler(
          &ServerSession::on_close,
          shared_from_this()));
#endif
}
#endif // 0

#if 0
void ServerSession::on_close(beast::error_code ec)
{
  if(ec)
      return on_session_fail(ec, "close");

  // If we get here then the connection is closed gracefully

  // The make_printable() function helps print a ConstBufferSequence
  // LOG(INFO) << beast::make_printable(buffer_.data());

  const ws::SessionGUID copyId = getId();
  RTC_DCHECK(!nm_->sessionManager().getSessById(copyId));
}
#endif // 0

void ServerSession::on_read(beast::error_code ec, std::size_t bytes_transferred) {
  // This means they closed the connection
  if(ec == beast::http::error::end_of_stream)
  {
      stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
      return;
  }

  // Handle the error, if any
  if(ec)
      return on_session_fail(ec, "read");

  // See if it is a WebSocket Upgrade
  if(websocket::is_upgrade(parser_->get()))
  {
      // Create a websocket session, transferring ownership
      // of both the socket and the HTTP request.
      parser_->release(); // TODO: transferring ownership of HTTP request.

      // Create the session and run it
      const auto newSessId = nextWsSessionId();

      RTC_DCHECK(ws_nm_);

      // NOTE: Following the std::move, the moved-from object is in the same state as if
      // constructed using the basic_stream_socket(io_service&) constructor.
      // boost.org/doc/libs/1_54_0/doc/html/boost_asio/reference/basic_stream_socket/basic_stream_socket/overload5.html
      auto newWSSession = std::make_shared<ws::ServerSession>(
        stream_.release_socket(), ctx_, ws_nm_, newSessId);
      ws_nm_->sessionManager().addSession(newSessId, newWSSession);

      if (!ws_nm_->sessionManager().onNewSessCallback_) {
        LOG(WARNING) << "HTTP: Not set onNewSessCallback_!";
        return;
      }

      ws_nm_->sessionManager().onNewSessCallback_(newWSSession);

      newWSSession->start_accept();
      return;
  }

    // Send the response
    //
    // The following code requires generic
    // lambdas, available in C++14 and later.
    //
    handle_request(
        "/",//state_->doc_root(),
        /*std::move*/(parser_->release()),
        [this](auto&& response)
        {
            // The lifetime of the message has to extend
            // for the duration of the async operation so
            // we use a shared_ptr to manage it.
            using response_type = typename std::decay<decltype(response)>::type;
            auto sp = boost::make_shared<response_type>(std::forward<decltype(response)>(response));
            // Write the response
            // NOTE: store `self` as separate variable due to ICE in gcc 7.3
            auto self = shared_from_this();
            beast::http::async_write(stream_, *sp,
                [self, sp](
                    beast::error_code ec, std::size_t bytes)
                {
                    self->on_write(ec, bytes, sp->need_eof());
                });
        });
}

void ServerSession::on_write(beast::error_code ec, std::size_t bytes_transferred, bool close) {
  // Handle the error, if any
  if(ec)
      return on_session_fail(ec, "write");

  http::SessionGUID copyId = getId();

  if(close)
  {
      // This means we should close the connection, usually because
      // the response indicated the "Connection: close" semantic.
      stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
      return;
  }

  // Read another request
  do_read();
}

/**
 * @brief starts async writing to client
 *
 * @param message message passed to client
 */
void ServerSession::send(const std::string& ss) {
  LOG(WARNING) << "NOTIMPLEMENTED: HTTP ServerSession::send:" << ss;
}

bool ServerSession::isOpen() const {
  LOG(WARNING) << "NOTIMPLEMENTED: HTTP ServerSession::isOpen";
  return true; // TODO
}

bool ServerSession::isExpired() const {
  LOG(WARNING) << "NOTIMPLEMENTED: HTTP ServerSession::isExpired";
  return true; // TODO
}

} // namespace http
} // namespace net
} // namespace gloer

#include "net/websockets/WsListener.hpp" // IWYU pragma: associated
#include "algo/StringUtils.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include "net/websockets/WsServer.hpp"
#include "net/websockets/WsSession.hpp"
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/assert.hpp>
#include <boost/beast/_experimental/core/ssl_stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/config.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/make_unique.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace gloer {
namespace net {

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace {

/** Return `true` if a buffer contains a TLS/SSL client handshake.
    This function returns `true` if the beginning of the buffer
    indicates that a TLS handshake is being negotiated, and that
    there are at least four octets in the buffer.
    If the content of the buffer cannot possibly be a TLS handshake
    request, the function returns `false`. Otherwise, if additional
    octets are required, `boost::indeterminate` is returned.
    @param buffer The input buffer to inspect. This type must meet
    the requirements of @b ConstBufferSequence.
    @return `boost::tribool` indicating whether the buffer contains
    a TLS client handshake, does not contain a handshake, or needs
    additional octets.
    @see
    http://www.ietf.org/rfc/rfc2246.txt
    7.4. Handshake protocol
*/
template <class ConstBufferSequence>
boost::tribool is_ssl_handshake(ConstBufferSequence const& buffers);

//]

//[example_core_detect_ssl_2

template <class ConstBufferSequence>
boost::tribool is_ssl_handshake(ConstBufferSequence const& buffers) {
  // Make sure buffers meets the requirements
  static_assert(boost::asio::is_const_buffer_sequence<ConstBufferSequence>::value,
                "ConstBufferSequence requirements not met");

  // Extract the first byte, which holds the
  // "message" type for the Handshake protocol.
  unsigned char v;
  if (boost::asio::buffer_copy(boost::asio::buffer(&v, 1), buffers) < 1) {
    // We need at least one byte to really do anything
    return boost::indeterminate;
  }

  // Check that the message type is "SSL Handshake" (rfc2246)
  if (v != 0x16) {
    // This is definitely not a handshake
    return false;
  }

  // At least four bytes are needed for the handshake
  // so make sure that we get them before returning `true`
  if (boost::asio::buffer_size(buffers) < 4)
    return boost::indeterminate;

  // This can only be a TLS/SSL handshake
  return true;
}

//]

//[example_core_detect_ssl_3

/** Detect a TLS/SSL handshake on a stream.
    This function reads from a stream to determine if a TLS/SSL
    handshake is being received. The function call will block
    until one of the following conditions is true:
    @li The disposition of the handshake is determined
    @li An error occurs
    Octets read from the stream will be stored in the passed dynamic
    buffer, which may be used to perform the TLS handshake if the
    detector returns true, or otherwise consumed by the caller based
    on the expected protocol.
    @param stream The stream to read from. This type must meet the
    requirements of @b SyncReadStream.
    @param buffer The dynamic buffer to use. This type must meet the
    requirements of @b DynamicBuffer.
    @param ec Set to the error if any occurred.
    @return `boost::tribool` indicating whether the buffer contains
    a TLS client handshake, does not contain a handshake, or needs
    additional octets. If an error occurs, the return value is
    undefined.
*/
template <class SyncReadStream, class DynamicBuffer>
boost::tribool detect_ssl(SyncReadStream& stream, DynamicBuffer& buffer,
                          boost::beast::error_code& ec) {
  namespace beast = boost::beast;

  // Make sure arguments meet the requirements
  static_assert(beast::is_sync_read_stream<SyncReadStream>::value,
                "SyncReadStream requirements not met");
  static_assert(boost::asio::is_dynamic_buffer<DynamicBuffer>::value,
                "DynamicBuffer requirements not met");

  // Loop until an error occurs or we get a definitive answer
  for (;;) {
    // There could already be data in the buffer
    // so we do this first, before reading from the stream.
    auto const result = is_ssl_handshake(buffer.data());

    // If we got an answer, return it
    if (!boost::indeterminate(result)) {
      // This is a fast way to indicate success
      // without retrieving the default category.
      ec = {};
      return result;
    }

    // The algorithm should never need more than 4 bytes
    BOOST_ASSERT(buffer.size() < 4);

    // Prepare the buffer's output area.
    auto const mutable_buffer = buffer.prepare(beast::read_size(buffer, 1536));

    // Try to fill our buffer by reading from the stream
    std::size_t const bytes_transferred = stream.read_some(mutable_buffer, ec);

    // Check for an error
    if (ec)
      break;

    // Commit what we read into the buffer's input area.
    buffer.commit(bytes_transferred);
  }

  // error
  return false;
}

//]

//[example_core_detect_ssl_4

/** Detect a TLS/SSL handshake asynchronously on a stream.
    This function is used to asynchronously determine if a TLS/SSL
    handshake is being received.
    The function call always returns immediately. The asynchronous
    operation will continue until one of the following conditions
    is true:
    @li The disposition of the handshake is determined
    @li An error occurs
    This operation is implemented in terms of zero or more calls to
    the next layer's `async_read_some` function, and is known as a
    <em>composed operation</em>. The program must ensure that the
    stream performs no other operations until this operation completes.
    Octets read from the stream will be stored in the passed dynamic
    buffer, which may be used to perform the TLS handshake if the
    detector returns true, or otherwise consumed by the caller based
    on the expected protocol.
    @param stream The stream to read from. This type must meet the
    requirements of @b AsyncReadStream.
    @param buffer The dynamic buffer to use. This type must meet the
    requirements of @b DynamicBuffer.
    @param handler Invoked when the operation completes.
    The handler may be moved or copied as needed.
    The equivalent function signature of the handler must be:
    @code
    void handler(
        error_code const& error,    // Set to the error, if any
        boost::tribool result       // The result of the detector
    );
    @endcode
    Regardless of whether the asynchronous operation completes
    immediately or not, the handler will not be invoked from within
    this function. Invocation of the handler will be performed in a
    manner equivalent to using `boost::asio::io_context::post`.
*/
template <class AsyncReadStream, class DynamicBuffer, class CompletionToken>
BOOST_ASIO_INITFN_RESULT_TYPE(/*< `BOOST_ASIO_INITFN_RESULT_TYPE` customizes the return value based
                                 on the completion token >*/
                              CompletionToken,
                              void(boost::beast::error_code,
                                   boost::tribool)) /*< This is the signature for the completion
                                                       handler >*/
async_detect_ssl(AsyncReadStream& stream, DynamicBuffer& buffer, CompletionToken&& token);

//]

//[example_core_detect_ssl_5

// This is the composed operation.
template <class AsyncReadStream, class DynamicBuffer, class Handler> class detect_ssl_op;

// Here is the implementation of the asynchronous initiation function
template <class AsyncReadStream, class DynamicBuffer, class CompletionToken>
BOOST_ASIO_INITFN_RESULT_TYPE(CompletionToken, void(boost::beast::error_code, boost::tribool))
async_detect_ssl(AsyncReadStream& stream, DynamicBuffer& buffer, CompletionToken&& token) {
  namespace beast = boost::beast;

  // Make sure arguments meet the requirements
  static_assert(beast::is_async_read_stream<AsyncReadStream>::value,
                "SyncReadStream requirements not met");
  static_assert(boost::asio::is_dynamic_buffer<DynamicBuffer>::value,
                "DynamicBuffer requirements not met");

  // This helper manages some of the handler's lifetime and
  // uses the result and handler specializations associated with
  // the completion token to help customize the return value.
  //
  boost::asio::async_completion<CompletionToken, void(beast::error_code, boost::tribool)> init{
      token};

  // Create the composed operation and launch it. This is a constructor
  // call followed by invocation of operator(). We use BOOST_ASIO_HANDLER_TYPE
  // to convert the completion token into the correct handler type,
  // allowing user defined specializations of the async result template
  // to take effect.
  //
  detect_ssl_op<AsyncReadStream, DynamicBuffer,
                BOOST_ASIO_HANDLER_TYPE(CompletionToken, void(beast::error_code, boost::tribool))>{
      stream, buffer, init.completion_handler}(beast::error_code{}, 0);

  // This hook lets the caller see a return value when appropriate.
  // For example this might return std::future<error_code, boost::tribool> if
  // CompletionToken is boost::asio::use_future.
  //
  // If a coroutine is used for the token, the return value from
  // this function will be the `boost::tribool` representing the result.
  //
  return init.result.get();
}

//]

//[example_core_detect_ssl_6

// Read from a stream to invoke is_tls_handshake asynchronously.
// This will be implemented using Asio's "stackless coroutines"
// which are based on macros forming a switch statement. The
// operation is derived from `coroutine` for this reason.
//
template <class AsyncReadStream, class DynamicBuffer, class Handler>
class detect_ssl_op : public boost::asio::coroutine {
  // This composed operation has trivial state,
  // so it is just kept inside the class and can
  // be cheaply copied as needed by the implementation.

  AsyncReadStream& stream_;

  // Boost.Asio and the Networking TS require an object of
  // type executor_work_guard<T>, where T is the type of
  // executor returned by the stream's get_executor function,
  // to persist for the duration of the asynchronous operation.
  boost::asio::executor_work_guard<decltype(std::declval<AsyncReadStream&>().get_executor())> work_;

  DynamicBuffer& buffer_;
  Handler handler_;
  boost::tribool result_ = false;

public:
  // Boost.Asio requires that handlers are CopyConstructible.
  // The state for this operation is cheap to copy.
  detect_ssl_op(detect_ssl_op const&) = default;

  // The constructor just keeps references the callers variables.
  //
  template <class DeducedHandler>
  detect_ssl_op(AsyncReadStream& stream, DynamicBuffer& buffer, DeducedHandler&& handler)
      : stream_(stream), work_(stream.get_executor()), buffer_(buffer),
        handler_(std::forward<DeducedHandler>(handler)) {}

  // Associated allocator support. This is Asio's system for
  // allowing the final completion handler to customize the
  // memory allocation strategy used for composed operation
  // states. A composed operation needs to use the same allocator
  // as the final handler. These declarations achieve that.

  using allocator_type = boost::asio::associated_allocator_t<Handler>;

  allocator_type get_allocator() const noexcept {
    return (boost::asio::get_associated_allocator)(handler_);
  }

  // Executor hook. This is Asio's system for customizing the
  // manner in which asynchronous completion handlers are invoked.
  // A composed operation needs to use the same executor to invoke
  // intermediate completion handlers as that used to invoke the
  // final handler.

  using executor_type =
      boost::asio::associated_executor_t<Handler,
                                         decltype(std::declval<AsyncReadStream&>().get_executor())>;

  executor_type get_executor() const noexcept {
    return (boost::asio::get_associated_executor)(handler_, stream_.get_executor());
  }

  // Our main entry point. This will get called as our
  // intermediate operations complete. Definition below.
  //
  void operator()(boost::beast::error_code ec, std::size_t bytes_transferred);
};

//]

//[example_core_detect_ssl_7

// detect_ssl_op is callable with the signature
// void(error_code, bytes_transferred),
// allowing `*this` to be used as a ReadHandler
//
template <class AsyncStream, class DynamicBuffer, class Handler>
void detect_ssl_op<AsyncStream, DynamicBuffer, Handler>::operator()(boost::beast::error_code ec,
                                                                    std::size_t bytes_transferred) {
  namespace beast = boost::beast;

  // This introduces the scope of the stackless coroutine
  BOOST_ASIO_CORO_REENTER(*this) {
    // There could already be data in the buffer
    // so we do this first, before reading from the stream.
    result_ = is_ssl_handshake(buffer_.data());

    // If we got an answer, return it
    if (!boost::indeterminate(result_)) {
      // We need to invoke the handler, but the guarantee
      // is that the handler will not be called before the
      // call to async_detect_ssl returns, so we must post
      // the operation to the executor. The helper function
      // `bind_handler` lets us bind arguments in a safe way
      // that preserves the type customization hooks of the
      // original handler.
      BOOST_ASIO_CORO_YIELD
      boost::asio::post(stream_.get_executor(), beast::bind_front_handler(std::move(*this), ec, 0));
    } else {
      // Loop until an error occurs or we get a definitive answer
      for (;;) {
        // The algorithm should never need more than 4 bytes
        BOOST_ASSERT(buffer_.size() < 4);

        BOOST_ASIO_CORO_YIELD {
          // Prepare the buffer's output area.
          auto const mutable_buffer = buffer_.prepare(beast::read_size(buffer_, 1536));

          // Try to fill our buffer by reading from the stream
          stream_.async_read_some(mutable_buffer, std::move(*this));
        }

        // Check for an error
        if (ec)
          break;

        // Commit what we read into the buffer's input area.
        buffer_.commit(bytes_transferred);

        // See if we can detect the handshake
        result_ = is_ssl_handshake(buffer_.data());

        // If it is detected, call the handler
        if (!boost::indeterminate(result_)) {
          // We don't need bind_handler here because we were invoked
          // as a result of an intermediate asynchronous operation.
          break;
        }
      }
    }

    // Invoke the final handler.
    handler_(ec, result_);
  }
}

// This function produces an HTTP response for the given
// request. The type of the response object depends on the
// contents of the request, so the interface requires the
// caller to pass a generic lambda for receiving the response.
template <class Body, class Allocator, class Send>
static void handle_request(boost::beast::string_view doc_root,
                           http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
  // Returns a bad request response
  auto const bad_request = [&req](boost::beast::string_view why) {
    http::response<http::string_body> res{http::status::bad_request, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = why.to_string();
    res.prepare_payload();
    return res;
  };

  // Returns a not found response
  auto const not_found = [&req](boost::beast::string_view target) {
    http::response<http::string_body> res{http::status::not_found, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "The resource '" + target.to_string() + "' was not found.";
    res.prepare_payload();
    return res;
  };

  // Returns a server error response
  auto const server_error = [&req](boost::beast::string_view what) {
    http::response<http::string_body> res{http::status::internal_server_error, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "An error occurred: '" + what.to_string() + "'";
    res.prepare_payload();
    return res;
  };

  // Make sure we can handle the method
  if (req.method() != http::verb::get && req.method() != http::verb::head)
    return send(bad_request("Unknown HTTP-method"));

  // Request path must be absolute and not contain "..".
  if (req.target().empty() || req.target()[0] != '/' ||
      req.target().find("..") != boost::beast::string_view::npos)
    return send(bad_request("Illegal request-target"));

  return send(bad_request("Unimplementd")); // TODO <<<<<<<<<
  /*
    // Build the path to the requested file
    std::string path = path_cat(doc_root, req.target());
    if (req.target().back() == '/')
      path.append("index.html");

    // Attempt to open the file
    boost::beast::error_code ec;
    http::file_body::value_type body;
    body.open(path.c_str(), boost::beast::file_mode::scan, ec);

    // Handle the case where the file doesn't exist
    if (ec == boost::system::errc::no_such_file_or_directory)
      return send(not_found(req.target()));

    // Handle an unknown error
    if (ec)
      return send(server_error(ec.message()));

    // Cache the size since we need it after the move
    auto const size = body.size();

    // Respond to HEAD request
    if (req.method() == http::verb::head) {
      http::response<http::empty_body> res{http::status::ok, req.version()};
      res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
      res.set(http::field::content_type, mime_type(path));
      res.content_length(size);
      res.keep_alive(req.keep_alive());
      return send(std::move(res));
    }

    // Respond to GET request
    http::response<http::file_body> res{std::piecewise_construct, std::make_tuple(std::move(body)),
                                        std::make_tuple(http::status::ok, req.version())};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mime_type(path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return send(std::move(res));*/
}

// TODO: prevent collision? respond ERROR to client if collided?
static std::string nextWsSessionId() { return gloer::algo::genGuid(); }

// Report a failure
void detect_session_fail(beast::error_code ec, char const* what) {
  std::cerr << what << ": " << ec.message() << "\n";
}

// Report a failure
void http_session_fail(beast::error_code ec, char const* what) {
  std::cerr << what << ": " << ec.message() << "\n";
}

// Handles an HTTP server connection.
// This uses the Curiously Recurring Template Pattern so that
// the same code works with both SSL streams and regular sockets.
template <class Derived> class http_session {
  // Access the derived class, this is part of
  // the Curiously Recurring Template Pattern idiom.
  Derived& derived() { return static_cast<Derived&>(*this); }

  // This queue is used for HTTP pipelining.
  class queue {
    enum {
      // Maximum number of responses we will queue
      limit = 8
    };

    // The type-erased, saved work item
    struct work {
      virtual ~work() = default;
      virtual void operator()() = 0;
    };

    http_session& self_;
    std::vector<std::unique_ptr<work>> items_;

  public:
    explicit queue(http_session& self) : self_(self) {
      static_assert(limit > 0, "queue limit must be positive");
      items_.reserve(limit);
    }

    // Returns `true` if we have reached the queue limit
    bool is_full() const { return items_.size() >= limit; }

    // Called when a message finishes sending
    // Returns `true` if the caller should initiate a read
    bool on_write() {
      BOOST_ASSERT(!items_.empty());
      auto const was_full = is_full();
      items_.erase(items_.begin());
      if (!items_.empty())
        (*items_.front())();
      return was_full;
    }

    // Called by the HTTP handler to send a response.
    template <bool isRequest, class Body, class Fields>
    void operator()(http::message<isRequest, Body, Fields>&& msg) {
      // This holds a work item
      struct work_impl : work {
        http_session& self_;
        http::message<isRequest, Body, Fields> msg_;

        work_impl(http_session& self, http::message<isRequest, Body, Fields>&& msg)
            : self_(self), msg_(std::move(msg)) {}

        void operator()() {
          http::async_write(self_.derived().stream(), msg_,
                            boost::asio::bind_executor(
                                self_.strand_, std::bind(&http_session::on_write,
                                                         self_.derived().shared_from_this(),
                                                         std::placeholders::_1, msg_.need_eof())));
        }
      };

      // Allocate and store the work
      items_.push_back(boost::make_unique<work_impl>(self_, std::move(msg)));

      // If there was no previous work, start this one
      if (items_.size() == 1)
        (*items_.front())();
    }
  };

  std::shared_ptr<std::string const> doc_root_;
  http::request<http::string_body> req_;
  queue queue_;

protected:
  boost::asio::steady_timer timer_;
  boost::asio::strand<boost::asio::io_context::executor_type> strand_;
  boost::beast::flat_buffer buffer_;
  NetworkManager* nm_;

public:
  // Construct the session
  http_session(boost::asio::io_context& ioc, boost::beast::flat_buffer buffer,
               std::shared_ptr<std::string const> const& doc_root, NetworkManager* nm)
      : doc_root_(doc_root), queue_(*this),
        timer_(ioc, (std::chrono::steady_clock::time_point::max)()), strand_(ioc.get_executor()),
        buffer_(std::move(buffer)), nm_(nm) {}

  void do_read() {
    // Set the timer
    timer_.expires_after(std::chrono::seconds(15));

    // Make the request empty before reading,
    // otherwise the operation behavior is undefined.
    req_ = {};

    // Read a request
    http::async_read(derived().stream(), buffer_, req_,
                     boost::asio::bind_executor(strand_, std::bind(&http_session::on_read,
                                                                   derived().shared_from_this(),
                                                                   std::placeholders::_1)));
  }

  // Called when the timer expires.
  void on_timer(boost::system::error_code ec) {
    if (ec && ec != boost::asio::error::operation_aborted)
      return http_session_fail(ec, "timer");

    // Verify that the timer really expired since the deadline may have moved.
    if (timer_.expiry() <= std::chrono::steady_clock::now())
      return derived().do_timeout();

    // Wait on the timer
    timer_.async_wait(boost::asio::bind_executor(
        strand_,
        std::bind(&http_session::on_timer, derived().shared_from_this(), std::placeholders::_1)));
  }

  void on_read(boost::system::error_code ec) {
    // Happens when the timer closes the socket
    if (ec == boost::asio::error::operation_aborted)
      return;

    // This means they closed the connection
    if (ec == http::error::end_of_stream)
      return derived().do_eof();

    if (ec)
      return http_session_fail(ec, "read");

    // See if it is a WebSocket Upgrade
    if (websocket::is_upgrade(req_)) {
      // Transfer the stream to a new WebSocket session
      // return make_websocket_session(derived().release_stream(), std::move(req_));
      // Launch SSL session
      // std::make_shared<ssl_session>(std::move(socket_), ctx_, std::move(buffer_),
      // doc_root_)->run();
      // Create the session and run it
      const auto newSessId = nextWsSessionId();
      // tcp::socket& socket = stream_; // std::move(derived().release_stream());
      /*boost::beast::websocket::stream<boost::beast::ssl_stream<boost::asio::ip::tcp::socket>>&&
          socketStream = std::move(derived().release_stream());*/

      auto newWsSession = std::make_shared<WsSession>(std::move(derived().release_stream()), nm_,
                                                      newSessId /*, nm_->getWS()->sslCtx_*/);
      nm_->getWS()->addSession(newSessId, newWsSession);
      newWsSession->run();
      LOG(INFO) << "new ws session " << newSessId;
      // newWsSession->send(std::make_shared<std::string>(welcomeMsg));
    }

    // Send the response
    handle_request(*doc_root_, std::move(req_), queue_);

    // If we aren't at the queue limit, try to pipeline another request
    if (!queue_.is_full())
      do_read();
  }

  void on_write(boost::system::error_code ec, bool close) {
    // Happens when the timer closes the socket
    if (ec == boost::asio::error::operation_aborted)
      return;

    if (ec)
      return http_session_fail(ec, "write");

    if (close) {
      // This means we should close the connection, usually because
      // the response indicated the "Connection: close" semantic.
      return derived().do_eof();
    }

    // Inform the queue that a write completed
    if (queue_.on_write()) {
      // Read another request
      do_read();
    }
  }
};

// Handles an SSL HTTP connection
class ssl_http_session : public http_session<ssl_http_session>,
                         public std::enable_shared_from_this<ssl_http_session> {
  boost::beast::ssl_stream<tcp::socket> stream_;
  boost::asio::strand<boost::asio::io_context::executor_type> strand_;
  bool eof_ = false;

public:
  // Create the http_session
  ssl_http_session(tcp::socket socket, ssl::context& ctx, boost::beast::flat_buffer buffer,
                   std::shared_ptr<std::string const> const& doc_root, NetworkManager* nm)
      : http_session<ssl_http_session>(socket.get_executor().context(), std::move(buffer), doc_root,
                                       nm),
        stream_(std::move(socket), ctx),
        // stream_(nm->getWS()->iocWsListener_->socket_, ctx),
        strand_(stream_.get_executor()) {}

  // Called by the base class
  boost::beast::ssl_stream<tcp::socket>& stream() { return stream_; }

  // Called by the base class
  boost::beast::ssl_stream<tcp::socket> release_stream() { return std::move(stream_); }

  // Start the asynchronous operation
  void run() {
    // Make sure we run on the strand
    if (!strand_.running_in_this_thread())
      return boost::asio::post(boost::asio::bind_executor(
          strand_, std::bind(&ssl_http_session::run, shared_from_this())));

    // Run the timer. The timer is operated
    // continuously, this simplifies the code.
    on_timer({});

    // Set the timer
    timer_.expires_after(std::chrono::seconds(15));

    // Perform the SSL handshake
    // Note, this is the buffered version of the handshake.
    stream_.async_handshake(
        ssl::stream_base::server, buffer_.data(),
        boost::asio::bind_executor(strand_,
                                   std::bind(&ssl_http_session::on_handshake, shared_from_this(),
                                             std::placeholders::_1, std::placeholders::_2)));
  }
  void on_handshake(boost::system::error_code ec, std::size_t bytes_used) {
    // Happens when the handshake times out
    if (ec == boost::asio::error::operation_aborted)
      return;

    if (ec)
      return http_session_fail(ec, "handshake");

    // Consume the portion of the buffer used by the handshake
    buffer_.consume(bytes_used);

    do_read();
  }

  void do_eof() {
    eof_ = true;

    // Set the timer
    timer_.expires_after(std::chrono::seconds(15));

    // Perform the SSL shutdown
    stream_.async_shutdown(
        boost::asio::bind_executor(strand_, std::bind(&ssl_http_session::on_shutdown,
                                                      shared_from_this(), std::placeholders::_1)));
  }

  void on_shutdown(boost::system::error_code ec) {
    // Happens when the shutdown times out
    if (ec == boost::asio::error::operation_aborted)
      return;

    if (ec)
      return http_session_fail(ec, "shutdown");

    // At this point the connection is closed gracefully
  }

  void do_timeout() {
    // If this is true it means we timed out performing the shutdown
    if (eof_)
      return;

    // Start the timer again
    timer_.expires_at((std::chrono::steady_clock::time_point::max)());
    on_timer({});
    do_eof();
  }
};

// Detects SSL handshakes
class detectSSL : public std::enable_shared_from_this<detectSSL> {
  tcp::socket socket_;
  ssl::context& ctx_;
  net::strand<net::io_context::executor_type> strand_;
  std::shared_ptr<std::string const> doc_root_;
  beast::flat_buffer buffer_;
  NetworkManager* nm_;

public:
  explicit detectSSL(tcp::socket socket, ssl::context& ctx,
                     std::shared_ptr<std::string const> const& doc_root, NetworkManager* nm)
      : socket_(std::move(socket)), ctx_(ctx),
        // strand_(nm_->getWS()->iocWsListener_->socket_.get_executor()),
        strand_(socket_.get_executor()), doc_root_(doc_root), nm_(nm) {}

  // Launch the detector
  void run() {
    async_detect_ssl(
        socket_,
        // nm_->getWS()->iocWsListener_->socket_,
        buffer_,
        net::bind_executor(strand_, std::bind(&detectSSL::on_detect, shared_from_this(),
                                              std::placeholders::_1, std::placeholders::_2)));
  }

  void on_detect(beast::error_code ec, boost::logic::tribool result) {
    if (ec)
      return detect_session_fail(ec, "detect");

    if (result) {
      // Launch SSL session
      std::make_shared<ssl_http_session>(std::move(socket_),
                                         // std::move(nm_->getWS()->iocWsListener_->socket_),
                                         ctx_, std::move(buffer_), doc_root_, nm_)
          ->run();
      // Launch SSL session
      // std::make_shared<ssl_session>(std::move(socket_), ctx_, std::move(buffer_),
      // doc_root_)->run();
      // Create the session and run it
      /*const auto newSessId = nextWsSessionId();
      auto newWsSession =
          std::make_shared<WsSession>(std::move(socket_), nm_, newSessId, ctx_, std::move(buffer_));
      nm_->getWS()->addSession(newSessId, newWsSession);
      newWsSession->run();
      std::string welcomeMsg = "welcome, ";
      welcomeMsg += newSessId;
      LOG(INFO) << "new ws session " << newSessId;*/
      // newWsSession->send(std::make_shared<std::string>(welcomeMsg));
      ///////newWsSession->send(welcomeMsg);
      return;
    }

    // Launch plain session
    // std::make_shared<plain_session>(std::move(socket_), std::move(buffer_), doc_root_)->run();
    LOG(WARNING) << "detectSSL: HTTP Session requires SSL!";
  }
};

} // namespace

WsListener::WsListener(boost::asio::ssl::context& ctx, boost::asio::io_context& ioc,
                       const boost::asio::ip::tcp::endpoint& endpoint,
                       std::shared_ptr<std::string const> doc_root, NetworkManager* nm)
    : acceptor_(ioc), socket_(ioc), doc_root_(doc_root), nm_(nm), endpoint_(endpoint), ctx_(ctx) {
  configureAcceptor();
}

// Report a failure
void WsListener::on_WsListener_fail(beast::error_code ec, char const* what) {
  LOG(WARNING) << "on_WsListener_fail: " << what << ": " << ec.message();
}

void WsListener::configureAcceptor() {
  beast::error_code ec;

  // Open the acceptor
  acceptor_.open(endpoint_.protocol(), ec);
  if (ec) {
    on_WsListener_fail(ec, "open");
    return;
  }

  // Allow address reuse
  acceptor_.set_option(net::socket_base::reuse_address(true), ec);
  if (ec) {
    on_WsListener_fail(ec, "set_option");
    return;
  }

  // Bind to the server address
  acceptor_.bind(endpoint_, ec);
  if (ec) {
    on_WsListener_fail(ec, "bind");
    return;
  }

  // Start listening for connections
  acceptor_.listen(net::socket_base::max_listen_connections, ec);
  if (ec) {
    on_WsListener_fail(ec, "listen");
    return;
  }
}

// Start accepting incoming connections
void WsListener::run() {
  LOG(INFO) << "WS run";
  if (!isAccepting())
    return;
  do_accept();
}

void WsListener::do_accept() {
  LOG(INFO) << "WS do_accept";
  acceptor_.async_accept(
      socket_, std::bind(&WsListener::on_accept, shared_from_this(), std::placeholders::_1));
}

/**
 * @brief handles new connections and starts sessions
 */
void WsListener::on_accept(beast::error_code ec) {
  LOG(INFO) << "WS on_accept";
  if (ec) {
    on_WsListener_fail(ec, "accept");
  } else {
    // Create the session and run it
    /*const auto newSessId = nextWsSessionId();
    auto newWsSession = std::make_shared<WsSession>(std::move(socket_), nm_, newSessId);
    nm_->getWS()->addSession(newSessId, newWsSession);
    newWsSession->run();
    std::string welcomeMsg = "welcome, ";
    welcomeMsg += newSessId;
    LOG(INFO) << "new ws session " << newSessId;
    // newWsSession->send(std::make_shared<std::string>(welcomeMsg));
    ///////newWsSession->send(welcomeMsg);
    ///*/

    // Create the detector http_session and run it
    // std::make_shared<detectSSL>(std::move(socket_), ctx_, doc_root_, nm_)->run();
    std::make_shared<detectSSL>(std::move(socket_), ctx_, doc_root_, nm_)->run();
  }

  // Accept another connection
  do_accept();
}

} // namespace net
} // namespace gloer

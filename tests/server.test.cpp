/*
 * Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include "algo/StringUtils.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "storage/path.hpp"
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <net/core.hpp>
#include <sstream>
#include <streambuf>
#include <string>
#include <thread>
#include <vector>

#include "testsCommon.h"

/*  Load a signed certificate into the ssl context, and configure
    the context for use with a server.

    For this to work with the browser or operating system, it is
    necessary to import the "Beast Test CA" certificate into
    the local certificate store, browser, or operating system
    depending on your environment Please see the documentation
    accompanying the Beast certificate for more details.
*/
inline void load_server_certificate(boost::asio::ssl::context& ctx) {
  /*
        The certificate was generated from CMD.EXE on Windows 10 using:

        winpty openssl dhparam -out dh.pem 2048
        winpty openssl req -newkey rsa:2048 -nodes -keyout key.pem -x509 -days 10000 -out cert.pem
     -subj "//C=US\ST=CA\L=Los Angeles\O=Beast\CN=www.example.com"
    */

  // see https://itnan.ru/post.php?c=1&p=271203

  const std::filesystem::path workDir = gloer::storage::getThisBinaryDirectoryPath();
  const std::filesystem::path assetsDir = (workDir / gloer::config::ASSETS_DIR);

  // openssl req -new -key user.key -out user.csr
  // openssl x509 -req -in user.csr -CA rootca.crt -CAkey rootca.key -CAcreateserial -out user.crt
  // -days 20000
  std::string const cert = gloer::storage::getFileContents(
      workDir / std::filesystem::path{"../gloer/assets/certs/server.crt"});

  // openssl genrsa -out user.key 2048
  std::string const key = gloer::storage::getFileContents(
      workDir / std::filesystem::path{"../gloer/assets/certs/server.key"});

  std::string const dh = gloer::storage::getFileContents(
      workDir / std::filesystem::path{"../gloer/assets/certs/dh.pem"});

  // openssl req -x509 -new -nodes -key rootCA.key -sha256 -days 10000 -out rootCA.pem
  // ctx.load_verify_file("../gloer/assets/certs/rootCA.pem");

  // password of certificate
  ctx.set_password_callback(
      [](std::size_t, boost::asio::ssl::context_base::password_purpose) { return "1234"; });

  ctx.set_options(boost::asio::ssl::context::default_workarounds |
                  boost::asio::ssl::context::no_sslv2 | boost::asio::ssl::context::single_dh_use);

  boost::system::error_code ec;
  ctx.use_certificate_chain(boost::asio::buffer(cert.data(), cert.size()), ec);
  if (ec) {
    LOG(WARNING) << "use_certificate_chain error: " << ec.message();
  }

  LOG(WARNING) << "cert: " << cert;

  ctx.use_private_key(boost::asio::buffer(key.data(), key.size()),
                      boost::asio::ssl::context::file_format::pem, ec);
  if (ec) {
    LOG(WARNING) << "use_private_key error: " << ec.message();
  }

  ctx.use_tmp_dh(boost::asio::buffer(dh.data(), dh.size()), ec);
  if (ec) {
    LOG(WARNING) << "use_tmp_dh error: " << ec.message();
  }
}

//------------------------------------------------------------------------------

// Report a failure
void fail(beast::error_code ec, char const* what) {
  // LOG(WARNING) << "failure: " << what << ": " << ec.message();
}

// Echoes back all received WebSocket messages
class session : public std::enable_shared_from_this<session> {
  ::tcp::socket socket_;
  websocket::stream<boost::asio::ssl::stream<::tcp::socket&>> ws_;
  ::net::strand<::net::io_context::executor_type> strand_;
  beast::multi_buffer buffer_;

public:
  // Take ownership of the socket
  session(::tcp::socket socket, boost::asio::ssl::context& ctx)
      : socket_(std::move(socket)), ws_(socket_, ctx), strand_(ws_.get_executor()) {}

  // Start the asynchronous operation
  void run() {
    LOG(WARNING) << "session run";
    // Perform the SSL handshake
    ws_.next_layer().async_handshake(
        boost::asio::ssl::stream_base::server,
        ::net::bind_executor(
            strand_, std::bind(&session::on_handshake, shared_from_this(), std::placeholders::_1)));
  }

  void on_handshake(beast::error_code ec) {
    LOG(WARNING) << "on_handshake";
    if (ec)
      return fail(ec, "handshake");

    // Accept the websocket handshake
    ws_.async_accept(::net::bind_executor(
        strand_, std::bind(&session::on_accept, shared_from_this(), std::placeholders::_1)));
  }

  void on_accept(beast::error_code ec) {
    LOG(WARNING) << "on_accept";
    if (ec)
      return fail(ec, "accept");

    // Read a message
    do_read();
  }

  void do_read() {
    // Read a message into our buffer
    ws_.async_read(buffer_, ::net::bind_executor(
                                strand_, std::bind(&session::on_read, shared_from_this(),
                                                   std::placeholders::_1, std::placeholders::_2)));
  }

  void on_read(beast::error_code ec, std::size_t bytes_transferred) {
    LOG(WARNING) << "on_read";
    boost::ignore_unused(bytes_transferred);

    // This indicates that the session was closed
    if (ec == websocket::error::closed)
      return;

    if (ec)
      fail(ec, "read");

    // Echo the message
    ws_.text(ws_.got_text());
    ws_.async_write(
        buffer_.data(),
        ::net::bind_executor(strand_, std::bind(&session::on_write, shared_from_this(),
                                                std::placeholders::_1, std::placeholders::_2)));
  }

  void on_write(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec)
      return fail(ec, "write");

    // Clear the buffer
    buffer_.consume(buffer_.size());

    // Do another read
    do_read();
  }
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener> {
  boost::asio::ssl::context& ctx_;
  ::tcp::acceptor acceptor_;
  ::tcp::socket socket_;

public:
  listener(::net::io_context& ioc, boost::asio::ssl::context& ctx, ::tcp::endpoint endpoint)
      : ctx_(ctx), acceptor_(ioc), socket_(ioc) {
    beast::error_code ec;

    // Open the acceptor
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
      fail(ec, "open");
      return;
    }

    // Allow address reuse
    acceptor_.set_option(::net::socket_base::reuse_address(true), ec);
    if (ec) {
      fail(ec, "set_option");
      return;
    }

    // Bind to the server address
    acceptor_.bind(endpoint, ec);
    if (ec) {
      fail(ec, "bind");
      return;
    }

    // Start listening for connections
    acceptor_.listen(::net::socket_base::max_listen_connections, ec);
    if (ec) {
      fail(ec, "listen");
      return;
    }
  }

  // Start accepting incoming connections
  void run() {
    if (!acceptor_.is_open())
      return;
    do_accept();
  }

  void do_accept() {
    acceptor_.async_accept(
        socket_, std::bind(&listener::on_accept, shared_from_this(), std::placeholders::_1));
  }

  void on_accept(beast::error_code ec) {
    if (ec) {
      fail(ec, "accept");
    } else {
      // Create the session and run it
      std::make_shared<session>(std::move(socket_), ctx_)->run();
    }

    // Accept another connection
    do_accept();
  }
};

//------------------------------------------------------------------------------

SCENARIO("ssltest", "[ssltest]") {

  auto const address = ::net::ip::make_address("0.0.0.0");
  unsigned short const port = 8080;
  int const threads = 1;

  // The io_context is required for all I/O
  ::net::io_context ioc{threads};

  // The SSL context is required, and holds certificates
  boost::asio::ssl::context ctx{boost::asio::ssl::context::sslv23};

  // This holds the self-signed certificate used by the server
  load_server_certificate(ctx);

  // Create and launch a listening port
  std::make_shared<listener>(ioc, ctx, ::tcp::endpoint{address, port})->run();

  // Run the I/O service on the requested number of threads
  /*std::vector<std::thread> v;
  v.reserve(threads - 1);
  for (auto i = threads - 1; i > 0; --i)
    v.emplace_back([&ioc] { ioc.run(); });
  ioc.run();*/
}

SCENARIO("matchers", "[matchers]") {
  REQUIRE_THAT("Hello olleH",
               Predicate<std::string>(
                   [](std::string const& str) -> bool { return str.front() == str.back(); },
                   "First and last character should be equal"));
}

SCENARIO("vectors can be sized and resized", "[vector]") {

  GIVEN("A vector with some items") {
    std::vector<int> v(5);

    REQUIRE(v.size() == 5);
    REQUIRE(v.capacity() >= 5);

    WHEN("the size is increased") {
      v.resize(10);

      THEN("the size and capacity change") {
        REQUIRE(v.size() == 10);
        REQUIRE(v.capacity() >= 10);
      }
    }
    WHEN("the size is reduced") {
      v.resize(0);

      THEN("the size changes but not capacity") {
        REQUIRE(v.size() == 0);
        REQUIRE(v.capacity() >= 5);
      }
    }
    WHEN("more capacity is reserved") {
      v.reserve(10);

      THEN("the capacity changes but not the size") {
        REQUIRE(v.size() == 5);
        REQUIRE(v.capacity() >= 10);
      }
    }
    WHEN("less capacity is reserved") {
      v.reserve(0);

      THEN("neither size nor capacity are changed") {
        REQUIRE(v.size() == 5);
        REQUIRE(v.capacity() >= 5);
      }
    }
  }
}

SCENARIO("stringUtils", "[algo]") {
  using namespace gloer::algo;

  GIVEN("trim_whitespace") {

    REQUIRE(trim_whitespace(std::string(" \n 1 2 34")) == "1 2 34");
    REQUIRE(trim_whitespace(std::string("1 2 34 \n ")) == "1 2 34");
    REQUIRE(trim_whitespace(std::string(" \n 1234")) == "1234");
    REQUIRE(trim_whitespace(std::string("1234 ")) == "1234");
    REQUIRE(trim_whitespace(std::string("1234")) == "1234");
    REQUIRE(trim_whitespace(std::string(" 1234 ")) == "1234 ");
    REQUIRE(trim_whitespace(std::string(" 1234 \n")) == "1234 ");
    REQUIRE(trim_whitespace(std::string("1234 \t")) == "1234");
    REQUIRE(trim_whitespace(std::string("\t")) == "");
  }
}

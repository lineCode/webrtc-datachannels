/*
 * Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

/** @file
 * @brief Game client start point
 */

#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
#include "algo/StringUtils.hpp"
#include "algo/TickManager.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManagerBase.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/wrtc/WRTCSession.hpp"
#include "net/wrtc/wrtc.hpp"
#include "net/ws/server/ServerSession.hpp"
#include "storage/path.hpp"
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/assert.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/system/error_code.hpp>
#include <chrono>
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <enum.h>

#ifndef __has_include
  static_assert(false, "__has_include not supported");
#else
#  if __has_include(<filesystem>)
#    include <filesystem>
     namespace fs = std::filesystem;
#  elif __has_include(<experimental/filesystem>)
#    include <experimental/filesystem>
     namespace fs = std::experimental::filesystem;
#  elif __has_include(<boost/filesystem.hpp>)
#    include <boost/filesystem.hpp>
     namespace fs = boost::filesystem;
#  endif
#endif

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <net/core.hpp>
#include <new>
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <rapidjson/error/error.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <webrtc/rtc_base/bind.h>
#include <webrtc/rtc_base/checks.h>
#include <webrtc/rtc_base/rtccertificategenerator.h>
#include <webrtc/rtc_base/ssladapter.h>

using namespace std::chrono_literals;

using namespace ::gloer::net;
/*using namespace ::gloer::net::http;
using namespace ::gloer::net::ws;
using namespace ::gloer::net::wrtc;
using namespace ::gloer::algo;

using namespace ::httpclient;*/

static void printNumOfCores() {
  unsigned int c = std::thread::hardware_concurrency();
  LOG(INFO) << "Number of cores: " << c;
  const size_t minCores = 4;
  if (c < minCores) {
    LOG(INFO) << "Too low number of cores! Prefer servers with at least " << minCores << " cores";
  }
}

int main(int argc, char* argv[]) {
  std::locale::global(std::locale::classic()); // https://stackoverflow.com/a/18981514/10904212

  printNumOfCores();

  // TODO https://stackoverflow.com/questions/49322387/boost-beast-memory-usage-for-bulk-requests

  /*
   * TODO: chunked
  https://www.boost.org/doc/libs/develop/libs/beast/doc/html/beast/using_http/chunked_encoding.html

TODO:
https://www.boost.org/doc/libs/develop/libs/beast/example/http/client/async-ssl/http_client_async_ssl.cpp
   *
  std::unique_ptr<HTTP::Client::ConnectionManager> http_client =
  std::make_unique<HTTP::Client::ConnectionManager>();
  http_client.set(HTTP::Request::Options::field::timeout, 2500ms);
  http_client.set(HTTP::Request::Options::field::base_URL, "https://api.example.com");
  http_client.set(HTTP::Request::Options::field::Authorization, "AUTH_TOKEN");
  http_client.set(HTTP::Request::Options::field::validateStatus,
                           [](const HTTP::StatusCode& status) {
                             // Reject only if the status code is greater than or equal to 500
                             return status < 500;
                           });

  HTTP::Proxy::Basic basicProxy("https://proxy.com");
  basicProxy.setUsername("Username");
  basicProxy.setPassword("Pass");

  http_client.set(HTTP::Request::Options::field::proxy, basicProxy);

  http_client.set(HTTP::Request::Options::field::on_connected,
                           [](const HTTP::Client::Connection& conn) {
                             // ...
                           });

  http_client.set(HTTP::Request::Options::field::on_disconnected, ( [](const
  HTTP::Client::Connection& conn) {
    // ...
    conn.reconnect();
  });

  std::unique_ptr<HTTP::Client::Connection> http_conn =
  http_client.connect("https://api.example.com");

  std::unique_ptr<HTTP::Client::Connection> http_conn2 =
  std::make_unique<HTTP::Client::Connection>(); http_client.addConnection(http_conn2);
  http_conn2.connect("https://api.example.com");

  // connection settings ovverides HTTP::Client::ConnectionManager settings
  http_conn.set(HTTP::Request::Options::field::base_URL, "https://api.example.com");
  http_conn.set(HTTP::Request::Options::field::Authorization, "AUTH_TOKEN");

  conn.on_connected,( []() {
    // ...
    if (conn.get(HTTP::Request::Options::field::base_URL) != "https://api.example.com") {
      // ...
    }
  });

  http_conn.on_disconnected,( []() {
    // ...
    if (!conn.isConnected()) {
      conn.reconnect();
    }
  });

  // Override timeout for this request as it's known to take a long time
  HTTP::Request::Options req_opts;
  req_opts.set(HTTP::Request::Options::field::timeout, 2500ms);
  req_opts.set(HTTP::Request::Options::field::data, "JSON_HERE");
  req_opts.set(HTTP::Request::Options::field::content_type, "application/x-www-form-urlencoded");

  // HTTP::Request settings ovverides HTTP::Client::ConnectionManager and HTTP::Client::Connection
  settings HTTP::Response res1 = http_conn.get("/longRequest", req_opts);

  req1.cancel();

  HTTP::Response res_resend = req1.resend();

  if (!http_client.isConnectedTo("https://api.example.com")) {
    http_conn.reconnect();
  }

  if (!http_conn.isConnected()) {
    http_conn.reconnect();
  }

  HTTP::Response res2 = http_conn.get("/longRequest", req_opts);
  if (res2.hasError()){
    const HTTP::Error err = res2.getError();
  }

  HTTP::Response res3 = http_conn.get("/longRequest", req_opts,
                  [](const HTTP::Response& res) {
    // ...
                  },
                  [](const HTTP::Error& err) {
    // ...
                  });

  req_opts.set(HTTP::Request::Options::field::use_future, true);
  std::future<HTTP::Response> res_future1 = http_conn.get("/longRequest", req_opts)
  .then([](const HTTP::Response& res) {
    // ...
  })
  .catch([](const HTTP::Error& err) {
    // ...
  });

  std::future_status status;
  do {
      status = res_future1.wait_for(std::chrono::seconds(0));

      if (status == std::future_status::deferred) {
          std::cout << "+";
      } else if (status == std::future_status::timeout) {
          std::cout << ".";
      }
  } while (status != std::future_status::ready);

   // see https://github.com/boostorg/beast/issues/1256
   req_opts.set(HTTP::Request::Options::field::use_future, true);
   std::future<HTTP::Response> res_future2 = http_conn.get("/longRequest", req_opts));
   req_opts.set(HTTP::Request::Options::field::data, "JSON_HERE");
   std::future<HTTP::Response> res_future3 = http_conn.post("/longRequest", req_opts));

    // Do other things here while the send completes.

    res_future2.get(); // Blocks until the send is complete. Throws any errors.
    res_future3.get(); // Blocks until the send is complete. Throws any errors.

*/
  return EXIT_SUCCESS;
}

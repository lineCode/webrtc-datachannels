#pragma once

#include <algorithm>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/config.hpp>
#include <boost/make_unique.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sol3/sol.hpp>
#include <string>
#include <type_traits>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace utils {
namespace config {

class ServerConfig {
public:
  ServerConfig(sol::state* luaScript) { loadFromScript(luaScript); };

  void print() const;

  void loadFromScript(sol::state* luaScript);

  // TODO private:
  net::ip::address address;
  unsigned short port;
  int32_t threads;
};

} // namespace config
} // namespace utils

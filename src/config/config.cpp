#include "config/config.hpp"
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/config.hpp>
#include <boost/make_unique.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <type_traits>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace {
std::string get_string_with_default(sol::state* luaScript,
                                    const std::string& key,
                                    const std::string& default_val) {
  if (!luaScript) {
    std::cout << "ServerConfig: invalid luaScript pointer \n";
  }

  auto val = (*luaScript)[key];
  if (val.get_type() == sol::type::nil || val.get_type() == sol::type::none) {
    std::cout << "ServerConfig: key " << key << " does not exist\n";
  }

  return luaScript->get_or<std::string>(key, default_val);
}
} // namespace

namespace utils {
namespace config {

void ServerConfig::print() const {
  std::cout << "address: " << address.to_string() << '\n'
            << "port: " << port << '\n'
            << "threads: " << threads << '\n';
}

void ServerConfig::loadFromScript(sol::state* luaScript) {
  if (!luaScript) {
    std::cout << "ServerConfig: invalid luaScript pointer \n";
  }

  address = net::ip::make_address(
      get_string_with_default(luaScript, "address", "127.0.0.1"));
  port = luaScript->get_or<unsigned short>("port", 8080);
  threads = std::atoi(luaScript->get_or<std::string>("threads", "1").c_str());
}

} // namespace config
} // namespace utils

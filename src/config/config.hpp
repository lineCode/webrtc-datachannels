#pragma once
#include <boost/asio/ip/address.hpp>
#include <sol3/sol.hpp>

namespace utils {
namespace config {

class ServerConfig {
public:
  ServerConfig(sol::state* luaScript) { loadFromScript(luaScript); };

  void print() const;

  void loadFromScript(sol::state* luaScript);

  // TODO private:
  boost::asio::ip::address address;
  unsigned short port;
  int32_t threads;
};

} // namespace config
} // namespace utils

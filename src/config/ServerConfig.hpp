#pragma once
#include <boost/asio.hpp>
#include <cstdint>
#include <string>

namespace sol {
class state;
} // namespace sol

namespace utils {
namespace config {

const std::string ASSETS_DIR = "assets";
const std::string CONFIG_NAME = "conf.lua";

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

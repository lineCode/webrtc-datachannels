#pragma once

#include <boost/asio.hpp>
#include <cstdint>
#include <filesystem>
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
  ServerConfig(sol::state* luaScript, const std::filesystem::path& workdir);

  ServerConfig(const std::filesystem::path& configPath,
               const std::filesystem::path& workdir);

  void print() const;

  void loadConfFromLuaScript(sol::state* luaScript);

  // TODO private:
  boost::asio::ip::address address_;

  // port for WebSockets connection
  unsigned short wsPort_;

  // port for WebRTC connection
  //  unsigned short wrtcPort;

  int32_t threads_;

  const std::filesystem::path workdir_;
};

} // namespace config
} // namespace utils

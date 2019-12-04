#pragma once

#include <cstdint>

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

#include <net/core.hpp>
#include <string>

namespace sol {
class state;
} // namespace sol

namespace gloer {
namespace config {

const std::string ASSETS_DIR = "assets";
const std::string CONFIGS_DIR = "configuration_files";
//const std::string CONFIG_NAME = "server_conf.lua";

struct ServerConfig {
  //ServerConfig(sol::state* luaScript, const fs::path& workdir);

  ServerConfig(const fs::path& configPath, const fs::path& workdir);

  void print() const;

  //void loadConfFromLuaScript(sol::state* luaScript);

  const fs::path workdir_;

  // TODO private:
  boost::asio::ip::address address_;

  // port for WebSockets connection
  unsigned short wsPort_;

  // port for WebRTC connection
  //  unsigned short wrtcPort;

  int32_t threads_;

  std::string cert_;
  std::string key_;
  std::string dh_;
  std::string certPass_;
};

} // namespace config
} // namespace gloer

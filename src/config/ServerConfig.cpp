#include "config/ServerConfig.hpp" // IWYU pragma: associated
#include "log/Logger.hpp"
//#include "lua/LuaScript.hpp"
#include "storage/path.hpp"
#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <string>

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

/*
namespace {
static std::string get_string_with_default(sol::state* luaScript, const std::string& key,
                                           const std::string& default_val) {
  if (!luaScript) {
    LOG(INFO) << "ServerConfig: invalid luaScript pointer";
    return "";
  }

  auto val = (*luaScript)[key];
  if (val.get_type() == sol::type::nil || val.get_type() == sol::type::none) {
    LOG(INFO) << "ServerConfig: key " << key << " does not exist";
  }

  return luaScript->get_or<std::string>(key, default_val);
}
} // namespace
*/
namespace gloer {
namespace config {

/*ServerConfig::ServerConfig(sol::state* luaScript, const ::fs::path& workdir) : workdir_(workdir) {
  loadConfFromLuaScript(luaScript);
}*/

ServerConfig::ServerConfig(const ::fs::path& configPath, const ::fs::path& workdir)
    : workdir_(workdir) {

  // get config from lua script or use defaults
  address_ = ::boost::asio::ip::make_address("127.0.0.1");
  wsPort_ = 8085;
  // TODO
  // wrtcPort
  threads_ = 1;

  const ::fs::path workDir = gloer::storage::getThisBinaryDirectoryPath();
  const ::fs::path assetsDir = (workDir / gloer::config::ASSETS_DIR);

  const ::fs::path certPath =
      ASSETS_DIR / ::fs::path("EMPTY certPath");
  if (!::fs::exists(certPath)) {
    LOG(WARNING) << "invalid certPath " << certPath;
    return;
  }

  const ::fs::path keyPath =
      ASSETS_DIR / ::fs::path("EMPTY keyPath");
  if (!::fs::exists(keyPath)) {
    LOG(WARNING) << "invalid keyPath " << keyPath;
    return;
  }

  const ::fs::path dhPath =
      ASSETS_DIR / ::fs::path("EMPTY dhPath");
  if (!::fs::exists(dhPath)) {
    LOG(WARNING) << "invalid dhPath " << dhPath;
    return;
  }

  cert_ = storage::getFileContents(certPath);
  key_ = storage::getFileContents(keyPath);
  dh_ = storage::getFileContents(dhPath);
  certPass_ = "";

  // load lua file
  /*gloer::lua::LuaScript luaScript;
  sol::state* configScript = luaScript.loadScriptFile(configPath);

  loadConfFromLuaScript(configScript);*/
}

void ServerConfig::print() const {
  LOG(INFO) << "address: " << address_.to_string() << '\n'
            << "port: " << wsPort_ << '\n'
            << "threads: " << threads_;
}

/*void ServerConfig::loadConfFromLuaScript(sol::state* luaScript) {
  if (!luaScript) {
    LOG(INFO) << "ServerConfig: invalid luaScript pointer";
  }

  // get config from lua script or use defaults
  address_ = ::boost::asio::ip::make_address(get_string_with_default(luaScript, "address", "127.0.0.1"));
  wsPort_ = luaScript->get_or<unsigned short>("port", 8085);
  // TODO
  // wrtcPort
  threads_ = std::atoi(luaScript->get_or<std::string>("threads", "1").c_str());

  const ::fs::path workDir = gloer::storage::getThisBinaryDirectoryPath();
  const ::fs::path assetsDir = (workDir / gloer::config::ASSETS_DIR);

  const ::fs::path certPath =
      ASSETS_DIR / ::fs::path(luaScript->get_or<std::string>("certPath", "EMPTY certPath"));
  if (!::fs::exists(certPath)) {
    LOG(WARNING) << "invalid certPath " << certPath;
    return;
  }

  const ::fs::path keyPath =
      ASSETS_DIR / ::fs::path(luaScript->get_or<std::string>("keyPath", "EMPTY keyPath"));
  if (!::fs::exists(keyPath)) {
    LOG(WARNING) << "invalid keyPath " << keyPath;
    return;
  }

  const ::fs::path dhPath =
      ASSETS_DIR / ::fs::path(luaScript->get_or<std::string>("dhPath", "EMPTY dhPath"));
  if (!::fs::exists(dhPath)) {
    LOG(WARNING) << "invalid dhPath " << dhPath;
    return;
  }

  cert_ = storage::getFileContents(certPath);
  key_ = storage::getFileContents(keyPath);
  dh_ = storage::getFileContents(dhPath);
  certPass_ = luaScript->get_or<std::string>("certPass", "");
}*/

} // namespace config
} // namespace gloer

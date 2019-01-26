#include "config/ServerConfig.hpp" // IWYU pragma: associated
#include "log/Logger.hpp"
#include "lua/LuaScript.hpp"
#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {
static std::string get_string_with_default(sol::state* luaScript, const std::string& key,
                                           const std::string& default_val) {
  if (!luaScript) {
    LOG(INFO) << "ServerConfig: invalid luaScript pointer";
  }

  auto val = (*luaScript)[key];
  if (val.get_type() == sol::type::nil || val.get_type() == sol::type::none) {
    LOG(INFO) << "ServerConfig: key " << key << " does not exist";
  }

  return luaScript->get_or<std::string>(key, default_val);
}
} // namespace

namespace gloer {
namespace config {

namespace fs = std::filesystem;         // from <filesystem>
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

ServerConfig::ServerConfig(sol::state* luaScript, const fs::path& workdir) : workdir_(workdir) {
  loadConfFromLuaScript(luaScript);
}

ServerConfig::ServerConfig(const fs::path& configPath, const fs::path& workdir)
    : workdir_(workdir) {
  // load lua file
  gloer::lua::LuaScript luaScript;
  sol::state* configScript = luaScript.loadScriptFile(configPath);

  loadConfFromLuaScript(configScript);
}

void ServerConfig::print() const {
  LOG(INFO) << "address: " << address_.to_string() << '\n'
            << "port: " << wsPort_ << '\n'
            << "threads: " << threads_;
}

void ServerConfig::loadConfFromLuaScript(sol::state* luaScript) {
  if (!luaScript) {
    LOG(INFO) << "ServerConfig: invalid luaScript pointer";
  }

  // get config from lua script or use defaults
  address_ = net::ip::make_address(get_string_with_default(luaScript, "address", "127.0.0.1"));
  wsPort_ = luaScript->get_or<unsigned short>("port", 8080);
  // TODO
  // wrtcPort
  threads_ = std::atoi(luaScript->get_or<std::string>("threads", "1").c_str());

  cert_ = luaScript->get_or<std::string>("cert", "");
  key_ = luaScript->get_or<std::string>("key", "");
  dh_ = luaScript->get_or<std::string>("dh", "");
}

} // namespace config
} // namespace gloer

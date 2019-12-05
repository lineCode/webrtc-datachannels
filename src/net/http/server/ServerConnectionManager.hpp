#pragma once

/**
 * \see https://www.boost.org/doc/libs/1_71_0/libs/beast/example/advanced/server-flex/advanced_server_flex.cpp
 **/

#include "net/http/SessionGUID.hpp"
#include "algo/CallbackManager.hpp"
#include <algorithm>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
//#include <boost/beast/experimental/core/ssl_stream.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/config.hpp>
#include <boost/make_unique.hpp>
#include <boost/beast/ssl.hpp>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <webrtc/rtc_base/criticalsection.h>
#include "net/http/server/ServerInputCallbacks.hpp"
#include "net/ConnectionManagerBase.hpp"
#include "net/ws/client/WSClientNetworkManager.hpp"
#include "net/ws/server/WSServerNetworkManager.hpp"

namespace gloer {
namespace net {

namespace http {
class ServerSessionManager;
class ServerInputCallbacks;
class HTTPServerNetworkManager;
} // namespace http

//class net::WSServerNetworkManager;

//class SessionBase;

namespace ws {
class Listener;
} // namespace ws

namespace http {
struct HTTPNetworkOperation;
//class ClientSession;
} // namespace http

} // namespace net
} // namespace gloer

namespace gloer {
namespace config {
struct ServerConfig;
} // namespace config
} // namespace gloer

namespace gloer {
namespace net {
namespace http {

/**
 * @brief manages currently valid sessions
 */
class ServerConnectionManager : public ConnectionManagerBase<http::SessionGUID> {
public:
  ServerConnectionManager(net::http::HTTPServerNetworkManager* http_nm,
    net::WSServerNetworkManager* ws_nm,
    const gloer::config::ServerConfig& serverConfig, http::ServerSessionManager& sm);

  // void interpret(size_t id, const std::string& message);

  void sendToAll(const std::string& message) override;

  void sendTo(const http::SessionGUID& sessionID, const std::string& message) override;

  // uint32_t getMaxSessionId() const { return maxSessionId_; }

  // TODO: limit max num of open sessions
  // uint32_t maxSessionId_ = 0;

  void run(const config::ServerConfig& serverConfig) override;

  void finish() override;

  void prepare(const config::ServerConfig& serverConfig);

  std::shared_ptr<ws::Listener> getListener() const;

  void addCallback(const http::HTTPNetworkOperation& op, const ServerNetworkOperationCallback& cb);

  boost::asio::io_context& getIOC() { return ioc_; }

private:
  void initListener(const config::ServerConfig& serverConfig);

private:
  // GameManager game_;

  // Run the I/O service on the requested number of threads
  std::vector<std::thread> httpThreads_;

  net::http::HTTPServerNetworkManager* http_nm_;

  net::WSServerNetworkManager* ws_nm_;

  std::shared_ptr<ws::Listener> HTTPAndWSListener_;

  // The io_context is required for all I/O
  boost::asio::io_context ioc_;

  http::ServerSessionManager& sm_;

  ::boost::asio::ssl::context ctx_;
};

} // namespace http
} // namespace net
} // namespace gloer

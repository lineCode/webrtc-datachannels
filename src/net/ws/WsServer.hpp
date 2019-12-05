#pragma once

/**
 * \see https://www.boost.org/doc/libs/1_71_0/libs/beast/example/advanced/server-flex/advanced_server_flex.cpp
 **/

#include "algo/CallbackManager.hpp"
#include "algo/NetworkOperation.hpp"
#include "net/ws/SessionManager.hpp"
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
#include "net/SessionPair.hpp"
#include "net/ws/Callbacks.hpp"
#include "net/ConnectionManagerBase.hpp"
#include "net/ws/SessionGUID.hpp"

namespace gloer {
namespace net {

//class net::WSServerNetworkManager;

//class SessionBase;
//class SessionPair;

namespace ws {
class WsListener;
class WsSession;
//class ClientSession;
} // namespace ws

} // namespace net
} // namespace gloer

namespace gloer {
namespace config {
struct ServerConfig;
} // namespace config
} // namespace gloer

namespace gloer {
namespace net {
namespace ws {

/**
 * @brief manages currently valid sessions
 */
class WSServer : public ConnectionManagerBase<ws::SessionGUID> {
public:
  WSServer(net::WSServerNetworkManager* nm, const gloer::config::ServerConfig& serverConfig, ws::ServerSessionManager& sm);

  // void interpret(size_t id, const std::string& message);

  void sendToAll(const std::string& message) override;

  void sendTo(const ws::SessionGUID& sessionID, const std::string& message) override;

  // uint32_t getMaxSessionId() const { return maxSessionId_; }

  // TODO: limit max num of open sessions
  // uint32_t maxSessionId_ = 0;

#if 0
  // TODO: limit max num of open connections per IP
  // uint32_t maxConnectionsPerIP_ = 0;
  std::shared_ptr<ClientSession> addClientSession(
    const ws::SessionGUID& newSessId);
#endif // 0

  void runThreads_t(const config::ServerConfig& serverConfig) override;

  void finishThreads_t() override;

  void prepare(const config::ServerConfig& serverConfig);

  std::shared_ptr<WsListener> getListener() const;

  void addCallback(const WsNetworkOperation& op, const WsServerNetworkOperationCallback& cb);

  boost::asio::io_context& getIOC() { return ioc_; }

private:
  void initListener(const config::ServerConfig& serverConfig);

private:
  // GameManager game_;

  // Run the I/O service on the requested number of threads
  std::vector<std::thread> wsThreads_;

  net::WSServerNetworkManager* nm_;

  std::shared_ptr<WsListener> wsListener_;

  // The io_context is required for all I/O
  boost::asio::io_context ioc_;

  ws::ServerSessionManager& sm_;

  ::boost::asio::ssl::context ctx_;
};

} // namespace ws
} // namespace net
} // namespace gloer

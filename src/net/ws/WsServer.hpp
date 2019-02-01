#pragma once

#include "algo/CallbackManager.hpp"
#include "algo/NetworkOperation.hpp"
#include "net/SessionManagerBase.hpp"
#include <algorithm>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/experimental/core/ssl_stream.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/config.hpp>
#include <boost/make_unique.hpp>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <rtc_base/criticalsection.h>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace gloer {
namespace net {

class NetworkManager;

namespace ws {
class WsSession;
class WsListener;
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

struct WsNetworkOperation : public algo::NetworkOperation<algo::WS_OPCODE> {
  WsNetworkOperation(const algo::WS_OPCODE& operationCode, const std::string& operationName)
      : NetworkOperation(operationCode, operationName) {}

  WsNetworkOperation(const algo::WS_OPCODE& operationCode) : NetworkOperation(operationCode) {}
};

typedef std::function<void(std::shared_ptr<WsSession> clientSession, NetworkManager* nm,
                           std::shared_ptr<std::string> messageBuffer)>
    WsNetworkOperationCallback;

class WSInputCallbacks
    : public algo::CallbackManager<WsNetworkOperation, WsNetworkOperationCallback> {
public:
  WSInputCallbacks();

  ~WSInputCallbacks();

  std::map<WsNetworkOperation, WsNetworkOperationCallback> getCallbacks() const override;

  void addCallback(const WsNetworkOperation& op, const WsNetworkOperationCallback& cb) override;
};

/**
 * @brief manages currently valid sessions
 */
class WSServer : public SessionManagerBase<WsSession, WSInputCallbacks> {
public:
  WSServer(NetworkManager* nm, const gloer::config::ServerConfig& serverConfig);

  // void interpret(size_t id, const std::string& message);

  void sendToAll(const std::string& message) override;

  void sendTo(const std::string& sessionID, const std::string& message) override;

  void unregisterSession(const std::string& id) override;

  // uint32_t getMaxSessionId() const { return maxSessionId_; }

  // TODO: limit max num of open sessions
  // uint32_t maxSessionId_ = 0;

  // TODO: limit max num of open connections per IP
  // uint32_t maxConnectionsPerIP_ = 0;

  void runThreads_t(const config::ServerConfig& serverConfig) override;

  void finishThreads_t() override;

  void runAsServer(const config::ServerConfig& serverConfig);

  // The io_context is required for all I/O
  boost::asio::io_context ioc_;

  void runAsClient(const config::ServerConfig& serverConfig);

  std::shared_ptr<WsListener> getListener() const;

private:
  void initListener(const config::ServerConfig& serverConfig);

private:
  // GameManager game_;

  // Run the I/O service on the requested number of threads
  std::vector<std::thread> wsThreads_;

  NetworkManager* nm_;

  std::shared_ptr<WsListener> wsListener_;
};

} // namespace ws
} // namespace net
} // namespace gloer

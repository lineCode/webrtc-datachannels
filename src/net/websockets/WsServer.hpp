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
#include <boost/beast/_experimental/core/ssl_stream.hpp>
#include <boost/beast/core.hpp>
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
class WsSession;
class NetworkManager;
class WsListener;
} // namespace net
} // namespace gloer

namespace gloer {
namespace config {
class ServerConfig;
} // namespace config
} // namespace gloer

namespace gloer {
namespace net {

struct WsNetworkOperation : public algo::NetworkOperation<algo::WS_OPCODE> {
  WsNetworkOperation(const algo::WS_OPCODE& operationCode, const std::string& operationName)
      : NetworkOperation(operationCode, operationName) {}

  WsNetworkOperation(const algo::WS_OPCODE& operationCode) : NetworkOperation(operationCode) {}
};

typedef std::function<void(WsSession* clientSession, NetworkManager* nm,
                           std::shared_ptr<std::string> messageBuffer)>
    WsNetworkOperationCallback;

/*typedef std::function<void(
    WRTCSession* clientSession,
    std::string_view messageBuffer)>
    WrtcNetworkOperationCallback;*/

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

  void handleIncomingMessages() override;

  void unregisterSession(const std::string& id) override;

  // uint32_t getMaxSessionId() const { return maxSessionId_; }

  // TODO: limit max num of open sessions
  // uint32_t maxSessionId_ = 0;

  // TODO: limit max num of open connections per IP
  // uint32_t maxConnectionsPerIP_ = 0;

  void runThreads(const config::ServerConfig& serverConfig) override;

  void finishThreads() override;

  void runIocWsListener(const config::ServerConfig& serverConfig);

  std::shared_ptr<WsListener> iocWsListener_;

private:
  // GameManager game_;

  // Run the I/O service on the requested number of threads
  std::vector<std::thread> wsThreads_;

  NetworkManager* nm_;

  // The io_context is required for all I/O
  boost::asio::io_context ioc_;
};

} // namespace net
} // namespace gloer

#pragma once

#include "net/SessionManagerI.hpp"
#include <algorithm/CallbackManager.hpp>
#include <algorithm/NetworkOperation.hpp>
#include <boost/beast/core.hpp>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <rtc_base/criticalsection.h>
#include <string>
#include <unordered_map>

namespace utils {
namespace net {
class WsSession;
class NetworkManager;
class WsListener;
} // namespace net
} // namespace utils

namespace utils {
namespace config {
class ServerConfig;
} // namespace config
} // namespace utils

namespace utils {
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
class WSServer : public SessionManagerI<WsSession, WSInputCallbacks> {
public:
  WSServer(NetworkManager* nm, const utils::config::ServerConfig& serverConfig);

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
} // namespace utils

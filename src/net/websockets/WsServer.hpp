#pragma once

#include <algorithm/CallbackManager.hpp>
#include <algorithm/NetworkOperation.hpp>
#include <boost/beast/core.hpp>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>

namespace utils {
namespace net {
class WsSession;
class NetworkManager;
} // namespace net
} // namespace utils

namespace utils {
namespace net {

struct WsNetworkOperation : public algo::NetworkOperation<algo::WS_OPCODE> {
  WsNetworkOperation(const algo::WS_OPCODE& operationCode, const std::string& operationName)
      : NetworkOperation(operationCode, operationName) {}

  WsNetworkOperation(const algo::WS_OPCODE& operationCode) : NetworkOperation(operationCode) {}
};

typedef std::function<void(utils::net::WsSession* clientSession, utils::net::NetworkManager* nm,
                           std::shared_ptr<boost::beast::multi_buffer> messageBuffer)>
    WsNetworkOperationCallback;

/*typedef std::function<void(
    utils::net::WRTCSession* clientSession,
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
class WSServer {
public:
  WSServer(NetworkManager* nm);

  void registerSession(const std::shared_ptr<WsSession>& session);

  void unregisterSession(const std::string& id);

  void interpret(size_t id, const std::string& message);

  void sendToAll(const std::string& message);

  void sendTo(const std::string& sessionID, const std::string& message);

  void handleAllPlayerMessages();

  void doToAllPlayers(std::function<void(std::shared_ptr<WsSession>)> func);
  /**
   * @brief returns the number of connected clients
   *
   * @return number of valid sessions
   */
  size_t getSessionsCount() const { return sessions_.size(); }

  size_t getSessions() const { return sessions_.size(); }

  std::shared_ptr<WsSession> getSessById(const std::string& wsConnId);

  // uint32_t getMaxSessionId() const { return maxSessionId_; }

  // TODO: limit max num of open sessions
  // uint32_t maxSessionId_ = 0;

  // TODO: limit max num of open connections per IP
  // uint32_t maxConnectionsPerIP_ = 0;

  WSInputCallbacks getWsOperationCallbacks() const;

private:
  std::unordered_map<std::string, std::shared_ptr<WsSession>> sessions_ = {};

  // GameManager game_;

  WSInputCallbacks wsOperationCallbacks_;

  NetworkManager* nm_;
};

} // namespace net
} // namespace utils

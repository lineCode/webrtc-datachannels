#pragma once

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

  // void interpret(size_t id, const std::string& message);

  ////////
  void sendToAll(const std::string& message);

  void sendTo(const std::string& sessionID, const std::string& message);

  void handleAllPlayerMessages();

  void doToAllSessions(std::function<void(std::shared_ptr<WsSession>)> func);

  /**
   * @brief returns the number of connected clients
   *
   * @return number of valid sessions
   */
  size_t getSessionsCount() const;

  std::unordered_map<std::string, std::shared_ptr<WsSession>> getSessions() const;

  std::shared_ptr<WsSession> getSessById(const std::string& sessionID);

  bool addSession(const std::string& sessionID, std::shared_ptr<WsSession> sess);

  void unregisterSession(const std::string& id);
  ///////

  // uint32_t getMaxSessionId() const { return maxSessionId_; }

  // TODO: limit max num of open sessions
  // uint32_t maxSessionId_ = 0;

  // TODO: limit max num of open connections per IP
  // uint32_t maxConnectionsPerIP_ = 0;

  WSInputCallbacks getWsOperationCallbacks() const;

private:
  rtc::CriticalSection sessionsMutex_;
  std::unordered_map<std::string, std::shared_ptr<WsSession>> sessions_ = {};

  // GameManager game_;

  WSInputCallbacks wsOperationCallbacks_;

  NetworkManager* nm_;
};

} // namespace net
} // namespace utils

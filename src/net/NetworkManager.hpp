#pragma once

#include "net/NetworkManager.hpp"
#include "net/WRTCServer.hpp"
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <thread>
#include <vector>

namespace utils {
namespace config {
class ServerConfig;
} // namespace config
} // namespace utils

namespace utils {
namespace net {

class WsSession;
class WsSessionManager;

struct NetworkOperation {
  NetworkOperation(uint32_t operationCode, const std::string& operationName)
      : operationCode_(operationCode),
        operationCodeStr_(std::to_string(operationCode)),
        operationName_(operationName) {}

  NetworkOperation(uint32_t operationCode)
      : operationCode_(operationCode),
        operationCodeStr_(std::to_string(operationCode)), operationName_("") {}

  const uint32_t operationCode_;
  const std::string operationCodeStr_;
  /**
   * operationName usefull for logging
   * NOTE: operationName may be empty
   **/
  const std::string operationName_;

  bool operator<(const NetworkOperation& rhs) const {
    return operationCode_ < rhs.operationCode_;
  }
};

const NetworkOperation PING_OPERATION = NetworkOperation(0, "PING");
const NetworkOperation CANDIDATE_OPERATION = NetworkOperation(1, "CANDIDATE");
const NetworkOperation OFFER_OPERATION = NetworkOperation(2, "OFFER");
const NetworkOperation ANSWER_OPERATION = NetworkOperation(3, "ANSWER");

typedef std::function<void(
    utils::net::WsSession* clientSession,
    std::shared_ptr<boost::beast::multi_buffer> messageBuffer)>
    WsNetworkOperationCallback;

class NetworkManager {
public:
  NetworkManager(const utils::config::ServerConfig& serverConfig);

  /*TODO: dynamic void addWSInputCallback(const NetworkOperation& op,
                          const NetworkOperationCallback& cb);*/

  std::shared_ptr<utils::net::WsSessionManager> getWsSessionManager() const;

  std::map<NetworkOperation, WsNetworkOperationCallback>
  getWsOperationCallbacks() const;

  void handleAllPlayerMessages();

  void runWsThreads(const utils::config::ServerConfig& serverConfig);

  void runWrtcThreads(const utils::config::ServerConfig& serverConfig);

  void finishWsThreads();

  void runIocWsListener(const utils::config::ServerConfig& serverConfig);

  void webRtcSignalThreadEntry(
      /*const utils::config::ServerConfig& serverConfig*/); // TODO

  std::shared_ptr<utils::net::WRTCServer> getWrtc() const {
    return wrtcServer_;
  }

private:
  std::map<NetworkOperation, WsNetworkOperationCallback> wsOperationCallbacks_;

  std::shared_ptr<utils::net::WsSessionManager> wssm_;

  std::shared_ptr<utils::net::WRTCServer> wrtcServer_;

  std::thread webrtc_thread_;

  // Run the I/O service on the requested number of threads
  std::vector<std::thread> wsThreads_;
  // The io_context is required for all I/O
  boost::asio::io_context ioc_;
};

} // namespace net
} // namespace utils

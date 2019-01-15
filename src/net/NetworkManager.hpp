#pragma once

#include "net/NetworkManager.hpp"
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

typedef std::function<void(
    utils::net::WsSession* clientSession,
    std::shared_ptr<boost::beast::multi_buffer> messageBuffer)>
    NetworkOperationCallback;

class NetworkManager {
public:
  NetworkManager(const utils::config::ServerConfig& serverConfig);

  /*TODO: dynamic void addWSInputCallback(const NetworkOperation& op,
                          const NetworkOperationCallback& cb);*/

  std::shared_ptr<utils::net::WsSessionManager> getWsSessionManager() const;

  std::map<NetworkOperation, NetworkOperationCallback>
  getWsOperationCallbacks() const;

  void handleAllPlayerMessages();

  void runWsThreads(const utils::config::ServerConfig& serverConfig);

  void finishWsThreads();

  void runIocWsListener(const utils::config::ServerConfig& serverConfig);

private:
  std::map<NetworkOperation, NetworkOperationCallback> wsOperationCallbacks_;
  std::shared_ptr<utils::net::WsSessionManager> wssm_;

  // Run the I/O service on the requested number of threads
  std::vector<std::thread> wsThreads_;
  // The io_context is required for all I/O
  boost::asio::io_context ioc_;
};

} // namespace net
} // namespace utils

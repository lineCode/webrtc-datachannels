#pragma once

#include "net/NetworkManager.hpp"
#include <boost/beast/core.hpp>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>

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
  NetworkManager();

  /*TODO: void addWSInputCallback(const NetworkOperation& op,
                          const NetworkOperationCallback& cb);*/

  std::shared_ptr<utils::net::WsSessionManager> getWsSessionManager() const;

  std::map<NetworkOperation, NetworkOperationCallback>
  getWsOperationCallbacks() const;

private:
  std::map<NetworkOperation, NetworkOperationCallback> wsOperationCallbacks_;
  std::shared_ptr<utils::net::WsSessionManager> sm_;
};

} // namespace net
} // namespace utils

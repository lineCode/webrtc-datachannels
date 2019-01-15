#pragma once

#include "net/NetworkManager.hpp"
#include <boost/asio/basic_datagram_socket.hpp> // IWYU pragma: keep
#include <boost/asio/basic_streambuf.hpp>       // IWYU pragma: keep
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
      : operationCode(operationCode), operationName(operationName),
        operationCodeStr(std::to_string(operationCode)) {}

  NetworkOperation(uint32_t operationCode)
      : operationCode(operationCode), operationName(""),
        operationCodeStr(std::to_string(operationCode)) {}

  const uint32_t operationCode;
  const std::string operationCodeStr;
  /**
   * operationName usefull for logging
   * NOTE: operationName may be empty
   **/
  const std::string operationName;

  bool operator<(const NetworkOperation& rhs) const {
    return operationCode < rhs.operationCode;
  }

  bool operator<(const uint32_t& rhsOperationCode) const {
    return operationCode < rhsOperationCode;
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

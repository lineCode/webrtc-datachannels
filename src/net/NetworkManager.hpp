#pragma once

#include "net/NetworkManager.hpp"
#include <boost/asio/basic_datagram_socket.hpp> // IWYU pragma: keep
#include <boost/asio/basic_streambuf.hpp>       // IWYU pragma: keep
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/websocket/error.hpp>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>

namespace utils {
namespace net {

class WsSession;
class WsSessionManager;

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

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

typedef std::function<void(utils::net::WsSession* clientSession,
                           std::shared_ptr<beast::multi_buffer> messageBuffer)>
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

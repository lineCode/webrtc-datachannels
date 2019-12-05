#include "net/http/server/ServerInputCallbacks.hpp" // IWYU pragma: associated
#include "algo/DispatchQueue.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManagerBase.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/wrtc/WRTCSession.hpp"
#include "net/wrtc/wrtc.hpp"
#include "net/http/server/ServerSession.hpp"
#include "net/http/server/ServerSessionManager.hpp"
#include "net/SessionBase.hpp"
#include "net/http/server/ServerSession.hpp"
#include "net/http/HTTPNetworkOperation.hpp"
#include "net/http/server/HTTPServerNetworkManager.hpp"
#include "net/http/server/ServerConnectionManager.hpp"
#include "net/http/server/ServerSessionManager.hpp"
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <cstddef>
#include <iostream>
#include <memory>
#include <net/core.hpp>
#include <rapidjson/document.h>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <webrtc/rtc_base/bind.h>
#include <webrtc/rtc_base/checks.h>

namespace gloer {
namespace net {
namespace http {

ServerInputCallbacks::ServerInputCallbacks() {}

ServerInputCallbacks::~ServerInputCallbacks() {}

std::map<http::HTTPNetworkOperation, ServerNetworkOperationCallback> ServerInputCallbacks::getCallbacks() const {
  return operationCallbacks_;
}

void ServerInputCallbacks::addCallback(const http::HTTPNetworkOperation& op,
                                   const ServerNetworkOperationCallback& cb) {
  operationCallbacks_[op] = cb;
}

} // namespace http
} // namespace net
} // namespace gloer

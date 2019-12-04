#include "net/ws/WsServer.hpp" // IWYU pragma: associated
#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManagerBase.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/wrtc/WRTCSession.hpp"
#include "net/wrtc/wrtc.hpp"
#include "net/ws/WsListener.hpp"
#include "net/ws/WsSession.hpp"
#include "net/ws/client/ClientSession.hpp"
#include "net/SessionBase.hpp"
#include "net/SessionPair.hpp"
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
namespace ws {

WSServerInputCallbacks::WSServerInputCallbacks() {}

WSServerInputCallbacks::~WSServerInputCallbacks() {}

std::map<WsNetworkOperation, WsServerNetworkOperationCallback> WSServerInputCallbacks::getCallbacks() const {
  return operationCallbacks_;
}

void WSServerInputCallbacks::addCallback(const WsNetworkOperation& op,
                                   const WsServerNetworkOperationCallback& cb) {
  operationCallbacks_[op] = cb;
}

/// ---

WSClientInputCallbacks::WSClientInputCallbacks() {}

WSClientInputCallbacks::~WSClientInputCallbacks() {}

std::map<WsNetworkOperation, WsClientNetworkOperationCallback> WSClientInputCallbacks::getCallbacks() const {
  return operationCallbacks_;
}

void WSClientInputCallbacks::addCallback(const WsNetworkOperation& op,
                                   const WsClientNetworkOperationCallback& cb) {
  operationCallbacks_[op] = cb;
}
} // namespace ws
} // namespace net
} // namespace gloer

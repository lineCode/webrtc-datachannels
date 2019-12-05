#include "net/ws/client/ClientInputCallbacks.hpp" // IWYU pragma: associated
#include "algo/DispatchQueue.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManagerBase.hpp"
#include "net/wrtc/WRTCSession.hpp"
#include "net/wrtc/wrtc.hpp"
#include "net/ws/client/ClientSession.hpp"
#include "net/ws/client/ClientSessionManager.hpp"
#include "net/SessionBase.hpp"
#include "net/ws/WsNetworkOperation.hpp"
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
#include "net/ws/WsNetworkOperation.hpp"

namespace gloer {
namespace net {
namespace ws {

ClientInputCallbacks::ClientInputCallbacks() {}

ClientInputCallbacks::~ClientInputCallbacks() {}

std::map<ws::WsNetworkOperation, ClientNetworkOperationCallback> ClientInputCallbacks::getCallbacks() const {
  return operationCallbacks_;
}

void ClientInputCallbacks::addCallback(const ws::WsNetworkOperation& op,
                                   const ClientNetworkOperationCallback& cb) {
  operationCallbacks_[op] = cb;
}
} // namespace ws
} // namespace net
} // namespace gloer

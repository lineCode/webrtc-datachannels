#pragma once

/**
 * \see https://www.boost.org/doc/libs/1_71_0/libs/beast/example/advanced/server-flex/advanced_server_flex.cpp
 **/

#include "algo/CallbackManager.hpp"
#include "algo/NetworkOperation.hpp"
#include <algorithm>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
//#include <boost/beast/experimental/core/ssl_stream.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/config.hpp>
#include <boost/make_unique.hpp>
#include <boost/beast/ssl.hpp>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <webrtc/rtc_base/criticalsection.h>
#include "net/SessionPair.hpp"
#include "net/NetworkManagerBase.hpp"

namespace gloer {
namespace net {

//class WSServerNetworkManager;

class SessionBase;
//class SessionPair;

namespace ws {
class WsListener;
class WsSession;
class ClientSession;
} // namespace ws

} // namespace net
} // namespace gloer

namespace gloer {
namespace config {
struct ServerConfig;
} // namespace config
} // namespace gloer

namespace gloer {
namespace net {
namespace ws {

struct WsNetworkOperation : public algo::NetworkOperation<algo::WS_OPCODE> {
  explicit WsNetworkOperation(const algo::WS_OPCODE& operationCode,
                              const std::string& operationName)
      : NetworkOperation(operationCode, operationName) {}

  explicit WsNetworkOperation(const algo::WS_OPCODE& operationCode)
      : NetworkOperation(operationCode) {}
};

typedef std::function<void(std::shared_ptr<SessionPair> clientSession, net::WSServerNetworkManager* nm,
                           std::shared_ptr<std::string> messageBuffer)>
    WsServerNetworkOperationCallback;

class WSServerInputCallbacks
    : public algo::CallbackManager<WsNetworkOperation, WsServerNetworkOperationCallback> {
public:
  WSServerInputCallbacks();

  ~WSServerInputCallbacks();

  std::map<WsNetworkOperation, WsServerNetworkOperationCallback> getCallbacks() const override;

  void addCallback(const WsNetworkOperation& op, const WsServerNetworkOperationCallback& cb) override;
};

typedef std::function<void(std::shared_ptr<SessionPair> clientSession, net::WSClientNetworkManager* nm,
                           std::shared_ptr<std::string> messageBuffer)>
    WsClientNetworkOperationCallback;

class WSClientInputCallbacks
    : public algo::CallbackManager<WsNetworkOperation, WsClientNetworkOperationCallback> {
public:
  WSClientInputCallbacks();

  ~WSClientInputCallbacks();

  std::map<WsNetworkOperation, WsClientNetworkOperationCallback> getCallbacks() const override;

  void addCallback(const WsNetworkOperation& op, const WsClientNetworkOperationCallback& cb) override;
};

} // namespace ws
} // namespace net
} // namespace gloer

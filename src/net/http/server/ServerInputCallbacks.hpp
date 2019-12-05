#pragma once

/**
 * \see https://www.boost.org/doc/libs/1_71_0/libs/beast/example/advanced/server-flex/advanced_server_flex.cpp
 **/

#include "net/http/SessionGUID.hpp"
#include "algo/CallbackManager.hpp"
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
#include "net/NetworkManagerBase.hpp"
#include <net/SessionBase.hpp>
//#include "net/http/server/HTTPServerNetworkManager.hpp"
#include "net/ws/client/WSClientNetworkManager.hpp"
#include "net/ws/server/WSServerNetworkManager.hpp"

namespace gloer {
namespace net {

namespace http {
/*class ServerConnectionManager;
class ServerSessionManager;
class ServerInputCallbacks;*/
class HTTPServerNetworkManager;
} // namespace http

//class WSServerNetworkManager;

//class SessionBase;

namespace http {
class ServerSession;
struct HTTPNetworkOperation;
class ServerSession;
} // namespace http

} // namespace net
} // namespace gloer

namespace gloer {
namespace config {
struct ServerConfig;
} // namespace config
} // namespace gloer

namespace gloer {
namespace net {
namespace http {

typedef std::function<void(std::shared_ptr<http::ServerSession> session,
                           net::http::HTTPServerNetworkManager* nm,
                           std::shared_ptr<std::string> messageBuffer)>
    ServerNetworkOperationCallback;

class ServerInputCallbacks
    : public algo::CallbackManager<http::HTTPNetworkOperation, ServerNetworkOperationCallback> {
public:
  ServerInputCallbacks();

  ~ServerInputCallbacks();

  std::map<http::HTTPNetworkOperation, ServerNetworkOperationCallback> getCallbacks() const override;

  void addCallback(const http::HTTPNetworkOperation& op, const ServerNetworkOperationCallback& cb) override;
};

} // namespace http
} // namespace net
} // namespace gloer

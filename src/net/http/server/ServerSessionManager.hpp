#pragma once

/**
 * \see https://www.boost.org/doc/libs/1_71_0/libs/beast/example/advanced/server-flex/advanced_server_flex.cpp
 **/

#include "algo/CallbackManager.hpp"
#include "net/SessionManagerBase.hpp"
#include "net/NetworkManagerBase.hpp"
#include <net/SessionBase.hpp>
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

namespace gloer {
namespace net {

//class WSServerNetworkManager;

//class SessionBase;

namespace http {
/*class ServerConnectionManager;
class ServerSessionManager;
class ServerInputCallbacks;*/
class HTTPServerNetworkManager;
class SessionGUID;
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

/**
 * @brief manages currently valid sessions
 */
class ServerSessionManager : public SessionManagerBase<http::ServerSession, http::SessionGUID> {
public:
  ServerSessionManager(net::http::HTTPServerNetworkManager* nm);

  void unregisterSession(const http::SessionGUID& id) override;

private:
  net::http::HTTPServerNetworkManager* nm_;
};

} // namespace http
} // namespace net
} // namespace gloer

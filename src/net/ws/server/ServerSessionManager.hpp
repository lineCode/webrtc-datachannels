#pragma once

/**
 * \see https://www.boost.org/doc/libs/1_71_0/libs/beast/example/advanced/server-flex/advanced_server_flex.cpp
 **/

#include "algo/CallbackManager.hpp"
#include "algo/NetworkOperation.hpp"
#include "net/SessionManagerBase.hpp"
#include "net/ws/server/ServerSessionManager.hpp"
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
#include "net/ws/Callbacks.hpp"
#include "net/NetworkManagerBase.hpp"
#include <net/SessionBase.hpp>

namespace gloer {
namespace net {

//class WSServerNetworkManager;

//class SessionBase;
//class SessionPair;

namespace ws {
class SessionGUID;
} // namespace ws

} // namespace net
} // namespace gloer

namespace gloer {
namespace config {
struct ServerConfig;
class SessionPair;
} // namespace config
} // namespace gloer

namespace gloer {
namespace net {
namespace ws {

/**
 * @brief manages currently valid sessions
 */
class ServerSessionManager : public SessionManagerBase<SessionPair, ws::SessionGUID> {
public:
  ServerSessionManager(net::WSServerNetworkManager* nm);

  void unregisterSession(const ws::SessionGUID& id) override;

private:
  net::WSServerNetworkManager* nm_;
};

} // namespace ws
} // namespace net
} // namespace gloer

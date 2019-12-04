#pragma once

/**
 * \see https://www.boost.org/doc/libs/1_71_0/libs/beast/example/advanced/server-flex/advanced_server_flex.cpp
 **/

#include "algo/CallbackManager.hpp"
#include "algo/NetworkOperation.hpp"
#include "net/SessionManagerBase.hpp"
#include "net/ws/SessionManager.hpp"
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
#include "net/SessionManagerBase.hpp"

namespace gloer {
namespace net {

class NetworkManager;

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
class SessionPair;
} // namespace config
} // namespace gloer

namespace gloer {
namespace net {
namespace ws {

/**
 * @brief manages currently valid sessions
 */
class SessionManager : public SessionManagerBase<SessionPair> {
public:
  SessionManager(NetworkManager* nm);

  void unregisterSession(const std::string& id) override;

private:
  NetworkManager* nm_;
};

} // namespace ws
} // namespace net
} // namespace gloer

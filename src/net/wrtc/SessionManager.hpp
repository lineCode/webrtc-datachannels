#pragma once

/**
 * \see https://www.boost.org/doc/libs/1_71_0/libs/beast/example/advanced/server-flex/advanced_server_flex.cpp
 **/

#include "algo/CallbackManager.hpp"
#include "net/SessionManagerBase.hpp"
#include "net/wrtc/SessionManager.hpp"
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
#include <net/SessionPair.hpp>
#include <net/wrtc/Callbacks.hpp>
#include <net/SessionManagerBase.hpp>
#include <net/NetworkManagerBase.hpp>
#include <net/wrtc/SessionGUID.hpp>

namespace gloer {
namespace net {

//class net::WRTCNetworkManager;

//class SessionBase;
//class SessionPair;

} // namespace net
} // namespace gloer

namespace wrtc {
class WRTCSession;
} // namespace wrtc

namespace gloer {
namespace config {
struct ServerConfig;
} // namespace config
} // namespace gloer

namespace gloer {
namespace net {
namespace wrtc {

/**
 * @brief manages currently valid sessions
 */
class SessionManager : public SessionManagerBase<WRTCSession, wrtc::SessionGUID> {
public:
  SessionManager(net::WRTCNetworkManager* nm);

  void unregisterSession(const wrtc::SessionGUID& id) override /*RTC_RUN_ON(signalingThread())*/;

private:
  net::WRTCNetworkManager* nm_;
};

} // namespace wrtc
} // namespace net
} // namespace gloer

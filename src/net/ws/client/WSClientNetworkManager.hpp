#pragma once

#include "net/http/SessionGUID.hpp"
#include "net/NetworkManagerBase.hpp"

#include <memory>

namespace gloer {
namespace config {
struct ServerConfig;
} // namespace config
} // namespace gloer

namespace gloer {
namespace net {

namespace http {
/*class ServerConnectionManager;
class ServerSessionManager;
class ServerInputCallbacks;*/
class HTTPServerNetworkManager;
} // namespace http

namespace ws {
class ServerConnectionManager;
class ClientConnectionManager;
class ClientSessionManager;
class ServerSessionManager;
class ClientInputCallbacks;
class ServerInputCallbacks;
}

namespace http {
class ServerConnectionManager;
class ServerSessionManager;
class ServerInputCallbacks;
}

namespace wrtc {
class WRTCServer;
class SessionManager;
class WRTCInputCallbacks;
}

} // namespace net
} // namespace gloer

namespace gloer {
namespace net {

typedef
  ::gloer::net::NetworkManager<
    ws::ClientConnectionManager, ws::ClientSessionManager, ws::ClientInputCallbacks
  > WSClientNetworkManager;

} // namespace net
} // namespace gloer

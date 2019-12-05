#pragma once

#include "net/http/SessionGUID.hpp"
#include "net/NetworkManagerBase.hpp"

#include "net/http/server/ServerConnectionManager.hpp"
#include "net/http/server/ServerSessionManager.hpp"
#include "net/http/server/ServerInputCallbacks.hpp"

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
namespace http {

class HTTPServerNetworkManager {
public:
  explicit HTTPServerNetworkManager(const gloer::config::ServerConfig& serverConfig, WSServerNetworkManager* ws_nm);

  void prepare(const gloer::config::ServerConfig& serverConfig);

  void run(const gloer::config::ServerConfig& serverConfig);

  void finish();

  std::shared_ptr<http::ServerConnectionManager> getRunner() const;

  http::ServerSessionManager& sessionManager();

  http::ServerInputCallbacks& operationCallbacks();

private:
  std::shared_ptr<http::ServerConnectionManager> sessionRunner_;

  http::ServerSessionManager sm_;

  http::ServerInputCallbacks operationCallbacks_{};

  WSServerNetworkManager* ws_nm_;
};

} // namespace http
} // namespace net
} // namespace gloer

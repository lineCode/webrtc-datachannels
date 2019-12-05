#include "net/http/server/HTTPServerNetworkManager.hpp" // IWYU pragma: associated
#include "net/NetworkManagerBase.hpp"
#include "net/http/server/ServerConnectionManager.hpp"
#include "net/http/server/ServerSessionManager.hpp"
#include "net/http/server/ServerInputCallbacks.hpp"

namespace gloer {
namespace net {
namespace http {

HTTPServerNetworkManager::HTTPServerNetworkManager(
  const gloer::config::ServerConfig &serverConfig
  , WSServerNetworkManager* ws_nm)
  : sm_(this)
  , ws_nm_(ws_nm) {
  RTC_DCHECK(ws_nm);
  // NOTE: no 'this' in constructor
  sessionRunner_ = std::make_shared<http::ServerConnectionManager>(this, ws_nm, serverConfig, sm_);
}

void HTTPServerNetworkManager::prepare(const gloer::config::ServerConfig &serverConfig) {
  RTC_DCHECK(sessionRunner_);
  sessionRunner_->prepare(serverConfig);
}

void HTTPServerNetworkManager::run(const gloer::config::ServerConfig &serverConfig) {
  RTC_DCHECK(sessionRunner_);
  sessionRunner_->run(serverConfig);
}

void HTTPServerNetworkManager::finish() {
  RTC_DCHECK(sessionRunner_);
  sessionRunner_->finish();
}

std::shared_ptr<gloer::net::http::ServerConnectionManager> HTTPServerNetworkManager::getRunner() const { return sessionRunner_; }

http::ServerSessionManager &HTTPServerNetworkManager::sessionManager() {
  return sm_;
}

http::ServerInputCallbacks &HTTPServerNetworkManager::operationCallbacks() { return operationCallbacks_; }

} // namespace http
} // namespace net
} // namespace gloer

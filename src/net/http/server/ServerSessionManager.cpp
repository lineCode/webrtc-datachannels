#include "net/http/server/ServerSessionManager.hpp" // IWYU pragma: associated
#include "net/http/server/ServerSession.hpp"
#include <net/http/SessionGUID.hpp>
#include "net/http/server/ServerConnectionManager.hpp"
#include "net/http/server/ServerInputCallbacks.hpp"
#include "net/http/server/HTTPServerNetworkManager.hpp"
#include "config/ServerConfig.hpp"

namespace gloer {
namespace net {
namespace http {

/**
 * @brief removes session from list of valid sessions
 *
 * @param id id of session to be removed
 */
void ServerSessionManager::unregisterSession(const http::SessionGUID& id) {
  LOG(WARNING) << "unregisterSession for id = " << static_cast<std::string>(id);
  const http::SessionGUID idCopy = id; // unknown lifetime, use idCopy
  std::shared_ptr<http::ServerSession> sess = getSessById(idCopy);

  {
    if (!removeSessById(idCopy)) {
      // LOG(WARNING) << "WsServer::unregisterSession: trying to unregister non-existing session "
      //             << idCopy;
      // NOTE: continue cleanup with saved shared_ptr
    }
    if (!sess) {
      // throw std::runtime_error(
      // LOG(WARNING) << "WsServer::unregisterSession: session already deleted";
      return;
    }
  }

  // close conn, e.t.c.
  /*if (sess && sess.get()) {
    sess->close();
  }*/

  // LOG(WARNING) << "WsServer: unregistered " << idCopy;
}

ServerSessionManager::ServerSessionManager(gloer::net::http::HTTPServerNetworkManager *nm)
  : nm_(nm)
{}

} // namespace http
} // namespace net
} // namespace gloer

#include "net/ws/server/ServerSessionManager.hpp" // IWYU pragma: associated
#include "net/SessionPair.hpp"
#include "net/ws/SessionGUID.hpp"
#include "net/ws/server/ServerConnectionManager.hpp"
#include "config/ServerConfig.hpp"

namespace gloer {
namespace net {
namespace ws {

/**
 * @brief removes session from list of valid sessions
 *
 * @param id id of session to be removed
 */
void ServerSessionManager::unregisterSession(const ws::SessionGUID& id) {
  LOG(WARNING) << "unregisterSession for id = " << static_cast<std::string>(id);
  const ws::SessionGUID idCopy = id; // unknown lifetime, use idCopy
  std::shared_ptr<SessionPair> sess = getSessById(idCopy);

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

ServerSessionManager::ServerSessionManager(gloer::net::WSServerNetworkManager *nm)
  : nm_(nm)
{}

} // namespace ws
} // namespace net
} // namespace gloer

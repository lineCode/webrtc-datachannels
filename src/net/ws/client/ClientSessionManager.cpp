#include "net/ws/client/ClientSessionManager.hpp" // IWYU pragma: associated
#include "net/SessionPair.hpp"
#include "net/ws/SessionGUID.hpp"
#include "net/ws/client/ClientConnectionManager.hpp"
#include "config/ServerConfig.hpp"
#include "net/ws/client/WSClientNetworkManager.hpp"
#include "net/ws/server/WSServerNetworkManager.hpp"

namespace gloer {
namespace net {
namespace ws {

/**
 * @brief removes session from list of valid sessions
 *
 * @param id id of session to be removed
 */
void ClientSessionManager::unregisterSession(const ws::SessionGUID& id) {
  LOG(WARNING) << "unregisterSession for id = " << static_cast<std::string>(id);
  const ws::SessionGUID idCopy = id; // unknown lifetime, use idCopy
  std::shared_ptr<SessionPair> sess = getSessById(idCopy);

  {
    if (!removeSessById(idCopy)) {
      // LOG(WARNING) << "ClientSessionManager::unregisterSession: trying to unregister non-existing session "
      //             << idCopy;
      // NOTE: continue cleanup with saved shared_ptr
    }
    if (!sess) {
      // throw std::runtime_error(
      // LOG(WARNING) << "ClientSessionManager::unregisterSession: session already deleted";
      return;
    }
  }

  // close conn, e.t.c.
  /*if (sess && sess.get()) {
    sess->close();
  }*/

  // LOG(WARNING) << "ClientSessionManager: unregistered " << idCopy;
}

ClientSessionManager::ClientSessionManager(gloer::net::WSClientNetworkManager *nm)
  : nm_(nm)
{}

} // namespace ws
} // namespace net
} // namespace gloer

#include "net/ws/SessionManager.hpp" // IWYU pragma: associated
#include "net/SessionManagerBase.hpp"
#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/wrtc/WRTCSession.hpp"
#include "net/wrtc/wrtc.hpp"
#include "net/ws/WsListener.hpp"
#include "net/ws/WsSession.hpp"
#include "net/ws/client/ClientSession.hpp"
#include "net/SessionBase.hpp"
#include "net/SessionPair.hpp"
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <cstddef>
#include <iostream>
#include <memory>
#include <net/core.hpp>
#include <net/SessionPair.hpp>
#include <rapidjson/document.h>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <webrtc/rtc_base/bind.h>
#include <webrtc/rtc_base/checks.h>

namespace gloer {
namespace net {
namespace ws {

/**
 * @brief removes session from list of valid sessions
 *
 * @param id id of session to be removed
 */
void SessionManager::unregisterSession(const std::string& id) {
  LOG(WARNING) << "unregisterSession for id = " << id;
  const std::string idCopy = id; // unknown lifetime, use idCopy
  std::shared_ptr<SessionPair> sess = getSessById(idCopy);

  // close conn, e.t.c.
  if (sess && sess.get()) {
    sess->close();
  }

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
  // LOG(WARNING) << "WsServer: unregistered " << idCopy;
}

SessionManager::SessionManager(gloer::net::NetworkManager *nm)
  : nm_(nm)
{}

} // namespace ws
} // namespace net
} // namespace gloer

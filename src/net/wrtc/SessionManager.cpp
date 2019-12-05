#include "net/wrtc/SessionManager.hpp" // IWYU pragma: associated
#include "net/SessionManagerBase.hpp"
#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManagerBase.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/wrtc/WRTCSession.hpp"
#include "net/wrtc/wrtc.hpp"
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
namespace wrtc {

/**
 * @brief removes session from list of valid sessions
 *
 * @param id id of session to be removed
 */
void SessionManager::unregisterSession(const wrtc::SessionGUID& id) {
  LOG(WARNING) << "unregisterSession for id = " << static_cast<std::string>(id);
  /*if (!signalingThread()->IsCurrent()) {
    return signalingThread()->Invoke<void>(RTC_FROM_HERE,
                                           [this, id] { return unregisterSession(id); });
  }

  RTC_DCHECK_RUN_ON(signalingThread());*/

  const wrtc::SessionGUID idCopy = id; // TODO: unknown lifetime, use idCopy
  std::shared_ptr<WRTCSession> sess = getSessById(idCopy);

  {
    if (!removeSessById(idCopy)) {
      // throw std::runtime_error(
      // LOG(WARNING) << "WRTCServer::unregisterSession: trying to unregister non-existing session "
      //             << idCopy;
      // NOTE: continue cleanup with saved shared_ptr
    }
    // LOG(WARNING) << "WrtcServer: unregistered " << idCopy;
    /*if (!sess || !sess.get()) {
      LOG(WARNING) << "WRTCServer::unregisterSession: session already deleted";
      return;
    }*/
  }

#if 0
  // note: close before removeSessById to keep dataChannelCount <= sessionsCount
  // close datachannel, pci, e.t.c.
  if (sess && sess.get()) {
    /*if (!signalingThread()->IsCurrent()) {
      signalingThread()->Invoke<void>(RTC_FROM_HERE,
                                      [sess] { return sess->close_s(false, false); });
    } else*/ {
      sess->close_s(false, false);
    }
  }
#endif // 0

  // LOG(WARNING) << "WRTCServer: unregistered " << idCopy;
}

SessionManager::SessionManager(gloer::net::WRTCNetworkManager *nm)
  : nm_(nm)
{}

} // namespace wrtc
} // namespace net
} // namespace gloer

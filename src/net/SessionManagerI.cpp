#include "net/SessionManagerI.hpp" // IWYU pragma: associated
#include "algorithm/CallbackManager.hpp"
#include "algorithm/NetworkOperation.hpp"
#include "algorithm/StringUtils.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include "net/webrtc/WRTCServer.hpp"
#include "net/webrtc/WRTCSession.hpp"
#include "net/websockets/WsServer.hpp"
#include "net/websockets/WsSession.hpp"
#include <algorithm>
#include <api/datachannelinterface.h>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <rapidjson/document.h>
#include <rtc_base/criticalsection.h>
#include <string>
#include <vector>
#include <webrtc/api/peerconnectioninterface.h>
#include <webrtc/p2p/client/basicportallocator.h>
#include <webrtc/rtc_base/scoped_ref_ptr.h>

namespace utils {
namespace net {

/*template <typename sessType = WsSession, typename callbacksType = WsNetworkOperationCallback>
void SessionManagerI<sessType, callbacksType>::sendToAll(const std::string& message) {
  static_assert(!std::is_base_of<sessType, SessionI>::value, "sessType must inherit from SessionI");
  LOG(WARNING) << "SessionManagerI::sendToAll:" << message;
  {
    for (auto& sessionkv : sessions_) {
      if (!sessionkv.second || !sessionkv.second.get()) {
        LOG(WARNING) << "WRTCServer::sendTo: Invalid session ";
        continue;
      }
      if (sessType session = sessionkv.second.get()) {
        session->send(message);
      }
    }
  }
}*/

/*template <>
void SessionManagerI<WsSession, WsNetworkOperationCallback>::sendToAll(const std::string& message) {
  LOG(WARNING) << "WS::sendToAll:" << message;
  {
    for (auto& sessionkv : sessions_) {
      if (!sessionkv.second || !sessionkv.second.get()) {
        LOG(WARNING) << "WS::sendTo: Invalid session ";
        continue;
      }
      if (auto session = sessionkv.second.get()) {
        session->send(message);
      }
    }
  }
}

template <>
void SessionManagerI<WRTCSession, WRTCNetworkOperationCallback>::sendToAll(
    const std::string& message) {
  LOG(WARNING) << "WRTCServer::sendToAll:" << message;
  {
    for (auto& sessionkv : sessions_) {
      if (!sessionkv.second || !sessionkv.second.get()) {
        LOG(WARNING) << "WRTCServer::sendTo: Invalid session ";
        continue;
      }
      if (auto session = sessionkv.second.get()) {
        session->send(message);
      }
    }
  }
}*/

/*// place this line at the end of your number.cpp file, in order to force that particular template
// class to be instantiated and fully compiled there.
// see https://stackoverflow.com/a/54133080/10904212
extern template class SessionManagerI<WsSession, WSInputCallbacks>;
extern template class SessionManagerI<WRTCSession, WRTCInputCallbacks>;*/

} // namespace net
} // namespace utils

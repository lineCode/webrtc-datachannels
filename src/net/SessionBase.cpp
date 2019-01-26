#include "algo/CallbackManager.hpp"
#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
#include "algo/StringUtils.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include "net/SessionManagerBase.hpp" // IWYU pragma: associated
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

namespace gloer {
namespace net {

SessionBase::SessionBase(const std::string& id) : id_(id) {}

std::shared_ptr<algo::DispatchQueue> SessionBase::getReceivedMessages() const {
  // NOTE: Returned smart pointer by value to increment reference count
  return receivedMessagesQueue_;
}

bool SessionBase::hasReceivedMessages() const {
  if (!receivedMessagesQueue_ && receivedMessagesQueue_.get()) {
    LOG(WARNING) << "WsSession::hasReceivedMessages invalid receivedMessagesQueue_";
    return true;
  }
  return receivedMessagesQueue_->isEmpty();
}

} // namespace net
} // namespace gloer

#include "net/wrtc/Callbacks.hpp" // IWYU pragma: associated
#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
#include "algo/StringUtils.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManagerBase.hpp"
#include "net/wrtc/Observers.hpp"
#include "net/wrtc/WRTCSession.hpp"
#include "net/wrtc/wrtc.hpp"
#include "net/SessionPair.hpp"
#include <api/call/callfactoryinterface.h>
#include <api/jsep.h>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <logging/rtc_event_log/rtc_event_log_factory_interface.h>
#include <p2p/base/portallocator.h>
#include <rapidjson/encodings.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <string>
#include <thread>
#include <webrtc/api/peerconnectioninterface.h>
#include <webrtc/base/compiler_specific.h>
#include <webrtc/base/macros.h>
#include <webrtc/media/base/mediaengine.h>
#include <webrtc/p2p/base/basicpacketsocketfactory.h>
#include <webrtc/p2p/client/basicportallocator.h>
#include <webrtc/pc/peerconnectionfactory.h>
#include <webrtc/rtc_base/asyncinvoker.h>
#include <webrtc/rtc_base/bind.h>
#include <webrtc/rtc_base/checks.h>
#include <webrtc/rtc_base/copyonwritebuffer.h>
#include <webrtc/rtc_base/messagehandler.h>
#include <webrtc/rtc_base/messagequeue.h>
#include <webrtc/rtc_base/rtccertificategenerator.h>
#include <webrtc/rtc_base/scoped_ref_ptr.h>
#include <webrtc/rtc_base/ssladapter.h>
#include <webrtc/rtc_base/thread.h>
/*
namespace rtc {

MessageHandler::~MessageHandler() { MessageQueueManager::Clear(this); }

} // namespace rtc
*/
namespace gloer {
namespace net {
namespace wrtc {

using namespace gloer::net::ws;

WRTCInputCallbacks::WRTCInputCallbacks() {}

WRTCInputCallbacks::~WRTCInputCallbacks() {}

std::map<WRTCNetworkOperation, WRTCNetworkOperationCallback>
WRTCInputCallbacks::getCallbacks() const {
  return operationCallbacks_;
}

void WRTCInputCallbacks::addCallback(const WRTCNetworkOperation& op,
                                     const WRTCNetworkOperationCallback& cb) {
  operationCallbacks_[op] = cb;
}

} // namespace wrtc
} // namespace net
} // namespace gloer

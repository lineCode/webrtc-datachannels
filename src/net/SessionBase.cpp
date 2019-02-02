#include "algo/CallbackManager.hpp"
#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
#include "algo/StringUtils.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include "net/SessionManagerBase.hpp" // IWYU pragma: associated
#include "net/wrtc/WRTCServer.hpp"
#include "net/wrtc/WRTCSession.hpp"
#include "net/ws/WsServer.hpp"
#include "net/ws/WsSession.hpp"
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
#include <string>
#include <vector>
#include <webrtc/api/peerconnectioninterface.h>
#include <webrtc/p2p/client/basicportallocator.h>
#include <webrtc/rtc_base/criticalsection.h>
#include <webrtc/rtc_base/scoped_ref_ptr.h>

namespace gloer {
namespace net {

SessionBase::SessionBase(const std::string& id) : id_(id) {}

} // namespace net
} // namespace gloer

#pragma once

#include "ServerManagerBase.hpp"
#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
#include "algo/TickManager.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/wrtc/WRTCSession.hpp"
#include "net/ws/WsListener.hpp"
#include "net/ws/WsServer.hpp"
#include "net/ws/WsSession.hpp"
#include "storage/path.hpp"
#include <algo/DispatchQueue.hpp>
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/system/error_code.hpp>
#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <enum.h>

#ifndef __has_include
  static_assert(false, "__has_include not supported");
#else
#  if __has_include(<filesystem>)
#    include <filesystem>
     namespace fs = std::filesystem;
#  elif __has_include(<experimental/filesystem>)
#    include <experimental/filesystem>
     namespace fs = std::experimental::filesystem;
#  elif __has_include(<boost/filesystem.hpp>)
#    include <boost/filesystem.hpp>
     namespace fs = boost::filesystem;
#  endif
#endif

#include <functional>
#include <iostream>
#include <map>
#include <net/core.hpp>
#include <new>
#include <rapidjson/document.h>
#include <rapidjson/error/error.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include <net/NetworkManagerBase.hpp>

namespace gloer {
namespace net {
//class NetworkManagerBase;
} // namespace net
} // namespace gloer

namespace gloer {
namespace net {
namespace wrtc {
class SessionGUID;
} // namespace wrtc
} // namespace net
} // namespace gloer

namespace gameserver {

using namespace ::gloer::algo;

class WRTCServerManager : public ServerManagerBase {
public:
  WRTCServerManager(std::weak_ptr<GameServer> game);

  void processIncomingMessages();

  bool handleIncomingJSON(const gloer::net::wrtc::SessionGUID& sessId, const std::string& message);

  void handleClose(const gloer::net::wrtc::SessionGUID& sessId);
};

} // namespace gameserver

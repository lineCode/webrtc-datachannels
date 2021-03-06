﻿#pragma once

#include "ServerManagerBase.hpp"
#include "algo/DispatchQueue.hpp"
#include "algo/TickManager.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManagerBase.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/wrtc/WRTCSession.hpp"
#include "net/ws/server/ServerSession.hpp"
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

namespace gloer {
namespace net {
namespace ws {
class SessionGUID;
} // namespace ws
} // namespace net
} // namespace gloer

namespace gameserver {

class WSServerManager : public ServerManagerBase {
public:
  WSServerManager(std::weak_ptr<GameServer> game);

  void processIncomingMessages();

  bool handleIncomingJSON(const gloer::net::ws::SessionGUID& sessId, const std::string& message);

  void handleClose(const gloer::net::ws::SessionGUID& sessId);
};

} // namespace gameserver

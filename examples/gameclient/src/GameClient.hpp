#pragma once

/** @file
 * @brief Class @ref GameClient
 */

#include "ServerManagerBase.hpp"
#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
#include "algo/TickManager.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/wrtc/WRTCSession.hpp"
#include "net/ws/client/ClientSession.hpp"
#include "storage/path.hpp"
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

namespace gameclient {

class WSServerManager;
class WRTCServerManager;

/**
 * @brief Manage game client
 */
class GameClient {
public:
  GameClient() {}

  static std::shared_ptr<
    ::gloer::net::WSClientNetworkManager
  > ws_nm;

  static std::shared_ptr<
    ::gloer::net::WRTCNetworkManager
  > wrtc_nm;

  void init(std::weak_ptr<GameClient> game);

  void handleIncomingMessages();

public:
  std::shared_ptr<WSServerManager> wsGameManager{nullptr};
  std::shared_ptr<WRTCServerManager> wrtcGameManager{nullptr};
};

} // namespace gameclient

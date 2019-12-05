#include "net/NetworkManagerBase.hpp" // IWYU pragma: associated
#include "config/ServerConfig.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/ws/client/ClientConnectionManager.hpp"
#include <api/peerconnectioninterface.h>
#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>

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

#include <string>
#include <thread>

namespace gloer {
namespace net {

using namespace gloer::net::ws;
using namespace gloer::net::wrtc;

} // namespace net
} // namespace gloer

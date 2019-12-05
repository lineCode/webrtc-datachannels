#include "ServerManagerBase.hpp" // IWYU pragma: associated

#include "config/ServerConfig.hpp"
#include "net/wrtc/WRTCServer.hpp"
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

namespace gameserver {

bool ServerManagerBase::hasReceivedMessages() const {
  if (!receivedMessagesQueue_ && receivedMessagesQueue_.get()) {
    LOG(WARNING) << "ServerManagerBase::hasReceivedMessages invalid receivedMessagesQueue_";
    return true;
  }
  return receivedMessagesQueue_->isEmpty();
}

std::shared_ptr<DispatchQueue> ServerManagerBase::getReceivedMessages() const {
  // NOTE: Returned smart pointer by value to increment reference count
  return receivedMessagesQueue_;
}

} // namespace gameserver

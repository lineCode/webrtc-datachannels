#include "ServerManagerBase.hpp" // IWYU pragma: associated

#include "config/ServerConfig.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/ws/WsListener.hpp"
#include "net/ws/WsServer.hpp"
#include <api/peerconnectioninterface.h>
#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <filesystem>
#include <string>
#include <thread>

namespace gameclient {

bool ServerManagerBase::hasReceivedMessages() const {
  if (!receivedMessagesQueue_ && receivedMessagesQueue_.get()) {
    LOG(WARNING) << "WsSession::hasReceivedMessages invalid receivedMessagesQueue_";
    return true;
  }
  return receivedMessagesQueue_->isEmpty();
}

std::shared_ptr<DispatchQueue> ServerManagerBase::getReceivedMessages() const {
  // NOTE: Returned smart pointer by value to increment reference count
  return receivedMessagesQueue_;
}

} // namespace gameclient

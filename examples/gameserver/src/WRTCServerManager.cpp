#include "WRTCServerManager.hpp" // IWYU pragma: associated

#include "GameServer.hpp"
#include "WRTCServerManager.hpp"
#include "algo/DispatchQueue.hpp"
#include "algo/TickManager.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/ws/server/ServerSessionManager.hpp"
#include "net/wrtc/SessionManager.hpp"
#include "net/NetworkManagerBase.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/wrtc/WRTCSession.hpp"
#include "net/ws/SessionGUID.hpp"
#include "net/wrtc/SessionGUID.hpp"
#include "storage/path.hpp"
#include <algorithm>
#include <api/peerconnectioninterface.h>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/assert.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/system/error_code.hpp>
#include <chrono>
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <enum.h>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <net/core.hpp>
#include <new>
#include <rapidjson/document.h>
#include <rapidjson/error/error.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

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

using namespace std::chrono_literals;

using namespace ::gloer::net::wrtc;
using namespace ::gloer::algo;

using namespace ::gameserver;

namespace gameserver {

/**
 * Type field stores uint32_t -> [0-4294967295] -> max 10 digits
 **/
constexpr unsigned long UINT32_FIELD_MAX_LEN = 10;

WRTCServerManager::WRTCServerManager(std::weak_ptr<GameServer> game) : ServerManagerBase(game) {
  LOG(WARNING) << "WRTCServerManager::WRTCServerManager";
  if (!receivedMessagesQueue_ || !receivedMessagesQueue_.get()) {
    LOG(WARNING) << "WRTCServerManager::WRTCServerManager: invalid receivedMessagesQueue_";
  }
  /*receivedMessagesQueue_ =
      std::make_shared<DispatchQueue>(std::string{"WRTCServerManager Server Dispatch Queue"}, 0);*/
}

void WRTCServerManager::processIncomingMessages() {
  if (game_.lock()->wrtc_nm->sessionManager().getSessionsCount()) {
    LOG(INFO) << "WRTCServerManager::handleIncomingMessages getSessionsCount "
              << game_.lock()->wrtc_nm->sessionManager().getSessionsCount();
    const std::unordered_map<gloer::net::wrtc::SessionGUID, std::shared_ptr<WRTCSession>>& sessions =
        game_.lock()->wrtc_nm->sessionManager().getSessions();
    /*std::string msg = "WRTC SESSIONS:[";
    for (auto& it : sessions) {
      std::shared_ptr<WRTCSession> wrtcs = it.second;
      msg += it.first;
      msg += "=";
      if (!wrtcs || !wrtcs.get()) {
        msg += "EMPTY";
      } else {
        msg += wrtcs->getId();
      }
    }
    msg += "]SESSIONS";
    LOG(INFO) << msg;*/
  }
  game_.lock()->wrtc_nm->sessionManager().doToAllSessions([&](const gloer::net::wrtc::SessionGUID& sessId,
                                                   std::shared_ptr<WRTCSession> session) {
    if (!session || !session.get()) {
      LOG(WARNING) << "WRTCServerManager::handleAllPlayerMessages: trying to "
                      "use non-existing session";
      // NOTE: unregisterSession must be automatic!
      game_.lock()->wrtc_nm->sessionManager().unregisterSession(sessId);
      return;
    }

    auto wrtcSessId = session->getId(); // remember id before session deletion

    if (session->fullyCreated() && !session->isDataChannelOpen()) {
      LOG(WARNING) << "WRTCServerManager::handleAllPlayerMessages: !session->isOpen()";
      // NOTE: unregisterSession must be automatic!
      game_.lock()->wrtc_nm->sessionManager().unregisterSession(wrtcSessId);
      return;
    }
    // TODO: check timer expiry independantly from handleIncomingMessages

    if (session->isExpired()) {
      LOG(WARNING) << "WRTCServerManager::handleAllPlayerMessages: session timer expired";
      game_.lock()->wrtc_nm->sessionManager().unregisterSession(wrtcSessId);
      return;
    }

    if (!receivedMessagesQueue_ || !receivedMessagesQueue_.get()) {
      LOG(WARNING)
          << "WRTCServerManager::handleAllPlayerMessages: invalid session->getReceivedMessages()";
      return;
    }
    // LOG(INFO) << "doToAllSessions for " << session->getId();

    /*auto nm = game_.lock()->wrtc_nm;
    if (nm->getRunner()->workerThread_.get()) {
      auto q = receivedMessagesQueue_;
      auto handle = OnceFunctor([q]() { q->DispatchQueued(); });
      nm->getRunner()->workerThread_->Post(RTC_FROM_HERE, handle);
    }*/

    receivedMessagesQueue_->DispatchQueued();
  });
}

/**
 * Add message to queue for further processing
 * Returs true if message can be processed
 **/
bool WRTCServerManager::handleIncomingJSON(const gloer::net::wrtc::SessionGUID& sessId, const std::string& message) {
  if (message.empty()) {
    LOG(WARNING) << "WRTCSession::handleIncomingJSON: invalid message";
    return false;
  }

  if (!receivedMessagesQueue_ || !receivedMessagesQueue_.get()) {
    LOG(WARNING) << "WRTCSession::on_read: invalid receivedMessagesQueue_ ";
    return false;
  }

  // parse incoming message
  rapidjson::Document message_object;
  rapidjson::ParseResult result = message_object.Parse(message.c_str());
  // LOG(INFO) << "incomingStr: " << message->c_str();
  if (!result || !message_object.IsObject() || !message_object.HasMember("type")) {
    LOG(WARNING) << "WRTCSession::on_read: ignored invalid message without type " << message;
    return false;
  }
  // Probably should do some error checking on the JSON object.
  std::string typeStr = message_object["type"].GetString();
  if (typeStr.empty() || typeStr.length() > UINT32_FIELD_MAX_LEN) {
    LOG(WARNING) << "WRTCSession::handleIncomingJSON: ignored invalid message with invalid "
                    "type field";
  }
  const auto& callbacks = game_.lock()->wrtc_nm->operationCallbacks().getCallbacks();

  const WRTCNetworkOperation wrtcNetworkOperation{
      static_cast<WRTC_OPCODE>(Opcodes::wrtcOpcodeFromStr(typeStr))};
  const auto itFound = callbacks.find(wrtcNetworkOperation);
  // if a callback is registered for event, add it to queue
  if (itFound != callbacks.end()) {
    WRTCNetworkOperationCallback callback = itFound->second;
    auto sessPtr = game_.lock()->wrtc_nm->sessionManager().getSessById(sessId);
    if (!sessPtr || !sessPtr.get()) {
      LOG(WARNING) << "WRTCSession::handleIncomingJSON: ignored invalid session";
      return false;
    }
    // WRTCSession* sess = sessPtr.get();
    DispatchQueue::dispatch_callback callbackBind = std::bind(
        callback, sessPtr, game_.lock()->wrtc_nm.get(), std::make_shared<std::string>(message));
    receivedMessagesQueue_->dispatch(callbackBind);
    // callbackBind();

    /*LOG(WARNING) << "WRTCSession::handleIncomingJSON: receivedMessagesQueue_->sizeGuess() "
                 << receivedMessagesQueue_->sizeGuess();*/
  } else {
    LOG(WARNING) << "WRTCSession::handleIncomingJSON: ignored invalid message with type "
                 << typeStr;
    return false;
  }

  return true;
}

void WRTCServerManager::handleClose(const gloer::net::wrtc::SessionGUID& sessId) {}

} // namespace gameserver

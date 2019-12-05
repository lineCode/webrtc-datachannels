#include "WSServerManager.hpp" // IWYU pragma: associated

#include "GameServer.hpp"
#include "WSServerManager.hpp"
#include "algo/DispatchQueue.hpp"
#include "algo/TickManager.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManagerBase.hpp"
#include "net/ws/server/ServerSessionManager.hpp"
#include "net/wrtc/SessionManager.hpp"
#include "net/ws/server/ServerInputCallbacks.hpp"
#include "net/ws/WsNetworkOperation.hpp"
#include "net/ws/server/ServerSession.hpp"
#include "net/ws/SessionGUID.hpp"
#include "net/SessionPair.hpp"
#include "storage/path.hpp"
#include <algorithm>
#include <api/peerconnectioninterface.h>
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
#include <new>
#include <rapidjson/document.h>
#include <rapidjson/error/error.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

using namespace ::gloer::net::ws;
using namespace ::gloer::algo;

using namespace ::gameserver;

namespace gameserver {

/**
 * Type field stores uint32_t -> [0-4294967295] -> max 10 digits
 **/
constexpr unsigned long UINT32_FIELD_MAX_LEN = 10;

WSServerManager::WSServerManager(std::weak_ptr<GameServer> game) : ServerManagerBase(game) {
  LOG(WARNING) << "WSServerManager::WSServerManager";
  if (!receivedMessagesQueue_ || !receivedMessagesQueue_.get()) {
    LOG(WARNING) << "WSServerManager::WSServerManager: invalid receivedMessagesQueue_";
  }
  /*receivedMessagesQueue_ =
      std::make_shared<DispatchQueue>(std::string{"WebSockets Server Dispatch Queue"}, 0);*/
}

/**
 * Add message to queue for further processing
 * Returs true if message can be processed
 **/
bool WSServerManager::handleIncomingJSON(const gloer::net::ws::SessionGUID& sessId, const std::string& message) {
  if (message.empty()) {
    LOG(WARNING) << "WS::handleIncomingJSON: invalid message";
    return false;
  }

  if (!receivedMessagesQueue_ || !receivedMessagesQueue_.get()) {
    LOG(WARNING) << "WS::handleIncomingJSON: invalid receivedMessagesQueue_ ";
    return false;
  }

  // parse incoming message
  rapidjson::Document message_object;
  rapidjson::ParseResult result = message_object.Parse(message.c_str());
  // LOG(INFO) << "incomingStr: " << message->c_str();
  if (!result || !message_object.IsObject() || !message_object.HasMember("type")) {
    LOG(WARNING) << "WsSession::handleIncomingJSON: ignored invalid message without type "
                 << message;
    return false;
  }
  // Probably should do some error checking on the JSON object.
  std::string typeStr = message_object["type"].GetString();
  if (typeStr.empty() || typeStr.length() > UINT32_FIELD_MAX_LEN) {
    LOG(WARNING) << "WsSession::handleIncomingJSON: ignored invalid message with invalid "
                    "type field";
  }

  /// TODO: nm <<<<
  const auto& callbacks = game_.lock()->ws_nm->operationCallbacks().getCallbacks();

  const gloer::net::ws::WsNetworkOperation NetworkOperation{
      static_cast<WS_OPCODE>(Opcodes::wsOpcodeFromStr(typeStr))};
  const auto itFound = callbacks.find(NetworkOperation);
  // if a callback is registered for event, add it to queue
  if (itFound != callbacks.end()) {
    gloer::net::ws::ServerNetworkOperationCallback callback = itFound->second;
    auto sessPtr = game_.lock()->ws_nm->sessionManager().getSessById(sessId);
    if (!sessPtr || !sessPtr.get()) {
      LOG(WARNING) << "WsSession::handleIncomingJSON: ignored invalid session";
      return false;
    }
    // WsSession* sess = sessPtr.get();
    DispatchQueue::dispatch_callback callbackBind = std::bind(
        callback, sessPtr, game_.lock()->ws_nm.get(), std::make_shared<std::string>(message));
    receivedMessagesQueue_->dispatch(callbackBind);

    /*LOG(WARNING) << "WsSession::handleIncomingJSON: receivedMessagesQueue_->sizeGuess() "
                 << receivedMessagesQueue_->sizeGuess();*/
  } else {
    LOG(WARNING) << "WsSession::handleIncomingJSON: ignored invalid message with type " << typeStr;
    return false;
  }

  return true;
}

void WSServerManager::handleClose(const gloer::net::ws::SessionGUID& sessId) {}

void WSServerManager::processIncomingMessages() {
  if (game_.lock()->ws_nm->sessionManager().getSessionsCount()) {
    LOG(INFO) << "WSServer::handleIncomingMessages getSessionsCount "
              << game_.lock()->ws_nm->sessionManager().getSessionsCount();
    const std::unordered_map<gloer::net::ws::SessionGUID, std::shared_ptr<gloer::net::SessionPair>>& sessions =
        game_.lock()->ws_nm->sessionManager().getSessions();
    /*std::string msg = "WS SESSIONS:[";
    for (auto& it : sessions) {
      std::shared_ptr<ws::ServerSession> wss = it.second;

    auto wsSessId = session->getId(); // remember id before session deletion

      msg += it.first;
      msg += "=";
      if (!wss || !wss.get()) {
        msg += "EMPTY";
      } else {
        msg += wss->getId();
      }
    }
    msg += "]SESSIONS";
    LOG(INFO) << msg;*/
  }
  game_.lock()->ws_nm->sessionManager().doToAllSessions([&](const gloer::net::ws::SessionGUID& sessId,
                                                 std::shared_ptr<gloer::net::SessionPair> session) {
    /*if (!session || !session.get()) {
      LOG(WARNING) << "WsServer::handleAllPlayerMessages: trying to "
                      "use non-existing session";
      // NOTE: unregisterSession must be automatic!
      game_.lock()->ws_nm->getRunner()->unregisterSession(sessId);
      return;
    }*/

    auto wsSessId = session->getId(); // remember id before session deletion

    /*if (!session->isOpen() && session->fullyCreated()) {
      LOG(WARNING) << "WsServer::handleAllPlayerMessages: !session->isOpen()";
      // NOTE: unregisterSession must be automatic!
      unregisterSession(session->getId());
      return;
    }*/

    /*if (session->isExpired()) {
      LOG(WARNING) << "WsServer::handleAllPlayerMessages: session timer expired";
      game_.lock()->ws_nm->getRunner()->unregisterSession(session->getId());
      return;
    }*/

    // LOG(INFO) << "doToAllSessions for " << session->getId();
    if (!receivedMessagesQueue_ || !receivedMessagesQueue_.get()) {
      LOG(WARNING) << "WsServer::handleAllPlayerMessages: invalid session->getReceivedMessages()";
      return;
    }
    receivedMessagesQueue_->DispatchQueued();
  });
}

} // namespace gameserver

/*
 * Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
#include "algo/TickManager.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/wrtc/WRTCSession.hpp"
#include "net/ws/WsServer.hpp"
#include "net/ws/WsSession.hpp"
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
#include <filesystem>
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

namespace fs = std::filesystem; // from <filesystem>

using namespace ::gloer::net::ws;
using namespace ::gloer::net::wrtc;
using namespace ::gloer::algo;

using namespace std::chrono_literals;

std::shared_ptr<::gloer::net::NetworkManager> nm;

static void printNumOfCores() {
  unsigned int c = std::thread::hardware_concurrency();
  LOG(INFO) << "Number of cores: " << c;
  const size_t minCores = 4;
  if (c < minCores) {
    LOG(INFO) << "Too low number of cores! Prefer servers with at least " << minCores << " cores";
  }
}

/**
 * Type field stores uint32_t -> [0-4294967295] -> max 10 digits
 **/
constexpr unsigned long UINT32_FIELD_MAX_LEN = 10;

/**
 * Add message to queue for further processing
 * Returs true if message can be processed
 **/
bool handleWSIncomingJSON(const std::string& sessId, const std::string& message) {
  if (message.empty()) {
    LOG(WARNING) << "WRTCSession::handleIncomingJSON: invalid message";
    return false;
  }

  // parse incoming message
  rapidjson::Document message_object;
  rapidjson::ParseResult result = message_object.Parse(message.c_str());
  // LOG(INFO) << "incomingStr: " << message->c_str();
  if (!result || !message_object.IsObject() || !message_object.HasMember("type")) {
    LOG(WARNING) << "WsSession::handleIncomingJSON: ignored invalid message without type";
    return false;
  }
  // Probably should do some error checking on the JSON object.
  std::string typeStr = message_object["type"].GetString();
  if (typeStr.empty() || typeStr.length() > UINT32_FIELD_MAX_LEN) {
    LOG(WARNING) << "WsSession::handleIncomingJSON: ignored invalid message with invalid "
                    "type field";
  }

  /// TODO: nm <<<<
  const auto& callbacks = nm->getWS()->getOperationCallbacks().getCallbacks();

  const WsNetworkOperation wsNetworkOperation =
      static_cast<WS_OPCODE>(Opcodes::wsOpcodeFromStr(typeStr));
  const auto itFound = callbacks.find(wsNetworkOperation);
  // if a callback is registered for event, add it to queue
  if (itFound != callbacks.end()) {
    WsNetworkOperationCallback callback = itFound->second;
    auto sessPtr = nm->getWS()->getSessById(sessId);
    if (!sessPtr || !sessPtr.get()) {
      LOG(WARNING) << "WsSession::handleIncomingJSON: ignored invalid session";
      return false;
    }
    WsSession* sess = sessPtr.get();
    DispatchQueue::dispatch_callback callbackBind =
        std::bind(callback, sess, nm.get(), std::make_shared<std::string>(message));
    auto receivedMessagesQueue_ = sess->getReceivedMessages();
    if (!receivedMessagesQueue_ || !receivedMessagesQueue_.get()) {
      LOG(WARNING) << "WsSession::handleIncomingJSON: invalid receivedMessagesQueue_ ";
      return false;
    }
    receivedMessagesQueue_->dispatch(callbackBind);

    /*LOG(WARNING) << "WsSession::handleIncomingJSON: receivedMessagesQueue_->sizeGuess() "
                 << receivedMessagesQueue_->sizeGuess();*/
  } else {
    LOG(WARNING) << "WsSession::handleIncomingJSON: ignored invalid message with type " << typeStr;
    return false;
  }

  return true;
}

void handleWSClose(const std::string& sessId) {}

/**
 * Add message to queue for further processing
 * Returs true if message can be processed
 **/
bool handleWRTCIncomingJSON(const std::string& sessId, const std::string& message) {
  if (message.empty()) {
    LOG(WARNING) << "WRTCSession::handleIncomingJSON: invalid message";
    return false;
  }

  // parse incoming message
  rapidjson::Document message_object;
  rapidjson::ParseResult result = message_object.Parse(message.c_str());
  // LOG(INFO) << "incomingStr: " << message->c_str();
  if (!result || !message_object.IsObject() || !message_object.HasMember("type")) {
    LOG(WARNING) << "WRTCSession::on_read: ignored invalid message without type";
    return false;
  }
  // Probably should do some error checking on the JSON object.
  std::string typeStr = message_object["type"].GetString();
  if (typeStr.empty() || typeStr.length() > UINT32_FIELD_MAX_LEN) {
    LOG(WARNING) << "WRTCSession::handleIncomingJSON: ignored invalid message with invalid "
                    "type field";
  }
  const auto& callbacks = nm->getWRTC()->getOperationCallbacks().getCallbacks();

  const WRTCNetworkOperation wrtcNetworkOperation =
      static_cast<WRTC_OPCODE>(Opcodes::wrtcOpcodeFromStr(typeStr));
  const auto itFound = callbacks.find(wrtcNetworkOperation);
  // if a callback is registered for event, add it to queue
  if (itFound != callbacks.end()) {
    WRTCNetworkOperationCallback callback = itFound->second;
    auto sessPtr = nm->getWRTC()->getSessById(sessId);
    if (!sessPtr || !sessPtr.get()) {
      LOG(WARNING) << "WsSession::handleIncomingJSON: ignored invalid session";
      return false;
    }
    WRTCSession* sess = sessPtr.get();
    DispatchQueue::dispatch_callback callbackBind =
        std::bind(callback, sess, nm.get(), std::make_shared<std::string>(message));
    auto receivedMessagesQueue_ = sess->getReceivedMessages();
    if (!receivedMessagesQueue_ || !receivedMessagesQueue_.get()) {
      LOG(WARNING) << "WRTCSession::handleIncomingJSON: invalid receivedMessagesQueue_ ";
      return false;
    }
    receivedMessagesQueue_->dispatch(callbackBind);

    /*LOG(WARNING) << "WRTCSession::handleIncomingJSON: receivedMessagesQueue_->sizeGuess() "
                 << receivedMessagesQueue_->sizeGuess();*/
  } else {
    LOG(WARNING) << "WRTCSession::handleIncomingJSON: ignored invalid message with type "
                 << typeStr;
    return false;
  }

  return true;
}

void handleWRTCClose(const std::string& sessId) {}

int main(int argc, char* argv[]) {

  size_t WRTCTickFreq = 100; // 1/Freq
  size_t WRTCTickNum = 0;

  size_t WSTickFreq = 100; // 1/Freq
  size_t WSTickNum = 0;

  gloer::log::Logger::instance(); // inits Logger

  printNumOfCores();

  const ::fs::path workdir = gloer::storage::getThisBinaryDirectoryPath();

  // TODO: support async file read, use futures or std::async
  // NOTE: future/promise Should Not Be Coupled to std::thread Execution Agents
  const gloer::config::ServerConfig serverConfig(
      ::fs::path{workdir / gloer::config::ASSETS_DIR / gloer::config::CONFIGS_DIR /
                 gloer::config::CONFIG_NAME},
      workdir);

  nm = std::make_shared<::gloer::net::NetworkManager>(serverConfig);

  // TODO: print active sessions

  // TODO: destroy inactive wrtc sessions (by timer)

  nm->runAsServer(serverConfig);

  nm->getWS()->SetOnNewSessionHandler([](std::shared_ptr<WsSession> sess) {
    sess->SetOnMessageHandler(handleWSIncomingJSON);
    sess->SetOnCloseHandler(handleWSClose);
  });

  nm->getWRTC()->SetOnNewSessionHandler([](std::shared_ptr<WRTCSession> sess) {
    sess->SetOnMessageHandler(handleWRTCIncomingJSON);
    sess->SetOnCloseHandler(handleWRTCClose);
  });

  LOG(INFO) << "Starting server loop for event queue";

  // processRecievedMsgs
  TickManager<std::chrono::milliseconds> tm(50ms);

  tm.addTickHandler(TickHandler("handleAllPlayerMessages", []() {
    // TODO: merge responses for same Player (NOTE: packet size limited!)

    // TODO: move game logic to separete thread or service

    // Handle queued incoming messages
    nm->handleIncomingMessages();
  }));

  {
    tm.addTickHandler(TickHandler("WSTick", [&WSTickFreq, &WSTickNum]() {
      WSTickNum++;
      if (WSTickNum < WSTickFreq) {
        return;
      } else {
        WSTickNum = 0;
      }
      LOG(WARNING) << "WSTick! " << nm->getWS()->getSessionsCount();
      // send test data to all players
      std::chrono::system_clock::time_point nowTp = std::chrono::system_clock::now();
      std::time_t t = std::chrono::system_clock::to_time_t(nowTp);
      std::string msg = "WS server_time: ";
      msg += std::ctime(&t);
      msg += ";Total WS connections:";
      msg += std::to_string(nm->getWS()->getSessionsCount());
      const std::unordered_map<std::string, std::shared_ptr<WsSession>>& sessions =
          nm->getWS()->getSessions();
      msg += ";SESSIONS:[";
      for (auto& it : sessions) {
        std::shared_ptr<WsSession> wss = it.second;
        msg += it.first;
        msg += "=";
        if (!wss || !wss.get()) {
          msg += "EMPTY";
        } else {
          msg += wss->getId();
        }
      }
      msg += "]SESSIONS";

      nm->getWS()->sendToAll(msg);
      nm->getWS()->doToAllSessions(
          [&](const std::string& sessId, std::shared_ptr<WsSession> session) {
            if (!session || !session.get()) {
              LOG(WARNING) << "WSTick: Invalid WsSession ";
              return;
            }
            session->send("Your WS id: " + session->getId());
          });
    }));
  }

  {
    tm.addTickHandler(TickHandler("WRTCTick", [&WRTCTickFreq, &WRTCTickNum]() {
      WRTCTickNum++;
      if (WRTCTickNum < WRTCTickFreq) {
        return;
      } else {
        WRTCTickNum = 0;
      }
      LOG(WARNING) << "WRTCTick! " << nm->getWRTC()->getSessionsCount();
      // send test data to all players
      std::chrono::system_clock::time_point nowTp = std::chrono::system_clock::now();
      std::time_t t = std::chrono::system_clock::to_time_t(nowTp);
      std::string msg = "WRTC server_time: ";
      msg += std::ctime(&t);
      msg += ";Total WRTC connections:";
      msg += std::to_string(nm->getWRTC()->getSessionsCount());
      const std::unordered_map<std::string, std::shared_ptr<WRTCSession>>& sessions =
          nm->getWRTC()->getSessions();
      msg += ";SESSIONS:[";
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

      nm->getWRTC()->sendToAll(msg);
      nm->getWRTC()->doToAllSessions(
          [&](const std::string& sessId, std::shared_ptr<WRTCSession> session) {
            if (!session || !session.get()) {
              LOG(WARNING) << "WRTCTick: Invalid WRTCSession ";
              return;
            }
            session->send("Your WRTC id: " + session->getId());
          });
    }));
  }

  while (tm.needServerRun()) {
    tm.tick();
  }

  // TODO: sendProcessedMsgs in separate thread

  // (If we get here, it means we got a SIGINT or SIGTERM)
  LOG(WARNING) << "If we get here, it means we got a SIGINT or SIGTERM";

  nm->finishServers();

  return EXIT_SUCCESS;
}

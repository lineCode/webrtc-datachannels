/*
 * Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
#include "algo/StringUtils.hpp"
#include "algo/TickManager.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/wrtc/WRTCSession.hpp"
#include "net/ws/WsListener.hpp"
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
#include <p2p/base/portallocator.h>
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <rapidjson/error/error.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
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

static std::string createTestMessage(std::string_view str) {
  using namespace ::gloer::net::ws;
  using namespace ::gloer::net::wrtc;
  using namespace ::gloer::algo;

  rapidjson::Document message_object;
  message_object.SetObject();
  message_object.AddMember("type", rapidjson::StringRef(Opcodes::opcodeToStr(WS_OPCODE::PING)),
                           message_object.GetAllocator());
  rapidjson::Value test_message_value;
  test_message_value.SetString(rapidjson::GenericStringRef<char>(str.data(), str.size()));
  rapidjson::Value message_payload;
  message_payload.SetObject();
  message_payload.AddMember("testMessageKey", test_message_value, message_object.GetAllocator());
  message_object.AddMember("payload", message_payload, message_object.GetAllocator());
  rapidjson::StringBuffer strbuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  bool done = message_object.Accept(writer);
  if (!done) {
    LOG(WARNING) << "OnIceCandidate: INVALID JSON!";
    return "";
  }
  return strbuf.GetString();
}

// TODO: add support for cleint-side webrtc

int main(int argc, char* argv[]) {
  using namespace ::gloer::net::ws;
  using namespace ::gloer::net::wrtc;
  using namespace ::gloer::algo;

  std::locale::global(std::locale::classic()); // https://stackoverflow.com/a/18981514/10904212

  gloer::log::Logger::instance(); // inits Logger

  LOG(INFO) << "Starting client..";

  printNumOfCores();

  uint32_t sendCounter = 0;

  const std::string clientNickname = gloer::algo::genGuid();

  const fs::path workdir = gloer::storage::getThisBinaryDirectoryPath();

  // TODO: support async file read, use futures or std::async
  // NOTE: future/promise Should Not Be Coupled to std::thread Execution Agents
  gloer::config::ServerConfig serverConfig(
      ::fs::path{workdir / gloer::config::ASSETS_DIR / gloer::config::CONFIGS_DIR /
                 gloer::config::CONFIG_NAME},
      workdir);

  // NOTE Tell the socket to bind to port 0 - random port
  serverConfig.wsPort_ = static_cast<unsigned short>(0);

  auto nm = std::make_shared<gloer::net::NetworkManager>(serverConfig);

  nm->runAsClient(serverConfig); // <<<<<

  /*nm->getWS()->SetOnNewSessionHandler([](std::shared_ptr<WsSession> sess) {
    sess->SetOnMessageHandler(handleWSIncomingJSON);
    sess->SetOnCloseHandler(handleWSClose);
  });

  nm->getWRTC()->SetOnNewSessionHandler([](std::shared_ptr<WRTCSession> sess) {
    sess->SetOnMessageHandler(handleWRTCIncomingJSON);
    sess->SetOnCloseHandler(handleWRTCClose);
  });*/

  // process recieved messages with some period
  TickManager<std::chrono::milliseconds> tm(50ms);

  // Create the session and run it
  const auto newSessId = "@clientSideServerId@";
  auto newWsSession = nm->getWS()->getListener()->addClientSession(newSessId);
  if (!newWsSession || !newWsSession.get()) {
    LOG(WARNING) << "addClientSession failed ";
    nm->finishServers();
    LOG(WARNING) << "exiting...";
    return EXIT_SUCCESS;
  }

  const std::string hostToConnect = "localhost";
  const std::string portToConnect = "8080";
  newWsSession->connectAsClient(hostToConnect, portToConnect);

  // NOTE: need wait connected state
  LOG(WARNING) << "connecting to " << hostToConnect << ":" << portToConnect << "...";
  bool isConnected = newWsSession->waitForConnect(/* maxWait_ms */ 5000);
  if (!isConnected) {
    LOG(WARNING) << "waitForConnect: Can`t connect to " << hostToConnect << ":" << portToConnect;
    nm->finishServers();
    LOG(WARNING) << "exiting...";
    return EXIT_SUCCESS;
  }
  LOG(WARNING) << "newWsSession->runAsClient...";
  newWsSession->runAsClient();

  tm.addTickHandler(TickHandler("handleAllPlayerMessages", [&nm, &newSessId, &newWsSession]() {
    // Handle queued incoming messages
    nm->handleIncomingMessages();
  }));

  tm.addTickHandler(
      TickHandler("sendTestMessage", [&nm, &sendCounter, &newWsSession, &clientNickname]() {
        const std::string msg =
            createTestMessage(std::to_string(sendCounter++) + " message from " + clientNickname);
        newWsSession->send(msg);
      }));

  while (tm.needServerRun()) {
    tm.tick();
  }

  // (If we get here, it means we got a SIGINT or SIGTERM)
  LOG(WARNING) << "If we get here, it means we got a SIGINT or SIGTERM";

  nm->finishServers();

  return EXIT_SUCCESS;
}

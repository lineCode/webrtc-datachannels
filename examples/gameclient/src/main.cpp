/*
 * Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

/** @file
 * @brief Game client start point
 */

#include "GameClient.hpp"
#include "WRTCServerManager.hpp"
#include "WSServerManager.hpp"
#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
#include "algo/StringUtils.hpp"
#include "algo/TickManager.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/SessionPair.hpp"
#include "net/NetworkManager.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/wrtc/WRTCSession.hpp"
#include "net/ws/WsListener.hpp"
#include "net/ws/WsServer.hpp"
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
//#include <folly/Singleton.h>
//#include <folly/init/Init.h>
#include "GameClient.hpp"
#include "WRTCServerManager.hpp"
#include "WSServerManager.hpp"
#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
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
//#include <folly/Singleton.h>
//#include <folly/init/Init.h>
#include "GameClient.hpp"
#include "WRTCServerManager.hpp"
#include "WSServerManager.hpp"
#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
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
#include <functional>
#include <iostream>
#include <map>
#include <new>
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
//#include <folly/Singleton.h>
//#include <folly/init/Init.h>
#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/wrtc/WRTCSession.hpp"
#include "net/wrtc/wrtc.hpp"
#include "net/ws/WsListener.hpp"
#include "net/ws/WsSession.hpp"
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <cstddef>
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
#include <webrtc/rtc_base/bind.h>
#include <webrtc/rtc_base/checks.h>
#include <webrtc/rtc_base/rtccertificategenerator.h>
#include <webrtc/rtc_base/ssladapter.h>
#include "net/ws/Callbacks.hpp"
#include "net/ws/client/Client.hpp"

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

namespace {

using namespace ::gloer::net;
using namespace ::gloer::net::wrtc;
using namespace ::gloer::net::ws;

static void pingCallback(std::shared_ptr<gloer::net::SessionPair> clientSession, NetworkManager* nm,
                         std::shared_ptr<std::string> messageBuffer) {
  if (!messageBuffer || !messageBuffer.get() || messageBuffer->empty()) {
    LOG(WARNING) << "WsServer: Invalid messageBuffer";
    return;
  }

  if (!clientSession || !clientSession.get()) {
    LOG(WARNING) << "WSServer invalid clientSession!";
    return;
  }

  if (!nm) {
    LOG(WARNING) << "WSServer invalid NetworkManager!";
    return;
  }

  std::string dataCopy = *messageBuffer.get();

  // const std::string incomingStr = beast::buffers_to_string(messageBuffer->data());
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "pingCallback incomingMsg=" << messageBuffer->c_str();

  // send same message back (ping-pong)
  if (clientSession && clientSession.get() && clientSession->isOpen() &&
      !clientSession->isExpired())
    clientSession->send(dataCopy);
}

/*

// Callback for when we receive a message from the server via the WebSocket.
function onWebSocketMessage(event) {
  console.log("onWebSocketMessage")
  let messageObject = "";
  try {
      messageObject = JSON.parse(event.data);
  } catch(e) {
      messageObject = event.data;
  }
  console.log("onWebSocketMessage type =", messageObject.type, ";event.data= ", event.data)
  if (messageObject.type === WS_PING_OPCODE) {
    const key = messageObject.payload;
    pingLatency[key] = performance.now() - pingTimes[key];
  } else if (messageObject.type === WS_ANSWER_OPCODE) {
    rtcPeerConnection.setRemoteDescription(new RTCSessionDescription(messageObject.payload));
  } else if (messageObject.type === WS_CANDIDATE_OPCODE) {
    rtcPeerConnection.addIceCandidate(new RTCIceCandidate(messageObject.payload));
  } else {
    console.log('Unrecognized WebSocket message type.', messageObject);
  }
}
*/

static void candidateCallback(std::shared_ptr<gloer::net::SessionPair> clientSession, NetworkManager* nm,
                              std::shared_ptr<std::string> messageBuffer) {
  // const std::string incomingStr = beast::buffers_to_string(messageBuffer->data());

  if (!messageBuffer || !messageBuffer.get() || messageBuffer->empty()) {
    LOG(WARNING) << "WsServer: Invalid messageBuffer";
    return;
  }

  if (!clientSession || !clientSession.get()) {
    LOG(WARNING) << "WSServer invalid clientSession!";
    return;
  }

  if (!nm) {
    LOG(WARNING) << "WSServer invalid NetworkManager!";
    return;
  }

  std::string dataCopy = *messageBuffer.get();

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "candidateCallback incomingMsg=" << dataCopy;

  // todo: pass parsed
  rapidjson::Document message_object;
  message_object.Parse(dataCopy.c_str());

  // Server receives Client’s ICE candidates, then finds its own ICE
  // candidates & sends them to Client
  LOG(INFO) << "type == candidate";

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "m_WS->WSQueue.dispatch type == candidate";
  auto spt =
      clientSession->getWRTCSession().lock(); // Has to be copied into a shared_ptr before usage

  /*auto handle = OnceFunctor([clientSession, spt, nm, &message_object1]() {
    if (spt) {
      spt->createAndAddIceCandidate(message_object1);
    } else {
      LOG(WARNING) << "wrtcSess_ expired";
      return;
    }
  });

  nm->getWRTC()->workerThread_->Post(RTC_FROM_HERE, handle);*/

  if (spt && spt.get()) {
    spt->createAndAddIceCandidateFromJson(message_object);
  } else {
    LOG(WARNING) << "wrtcSess_ expired";
    return;
  }

  // webSocketConnection.send(JSON.stringify({type: WS_CANDIDATE_OPCODE, payload:
  // event.candidate})); clientSession->send(message_object1);

  /*rapidjson::Document message_object;
  message_object.SetObject();
  rapidjson::Value type;
  message_object.AddMember("type",
                           rapidjson::StringRef(algo::Opcodes::opcodeToStr(algo::WS_OPCODE::OFFER)),
                           message_object.GetAllocator());
  rapidjson::Value sdp_value;
  sdp_value.SetString(rapidjson::StringRef(offer_string.c_str()));

  rapidjson::Value message_payload;
  message_payload.SetObject();
  message_payload.AddMember(
      "type",
      kOfferSdpName, // rapidjson::StringRef(ANSWER_OPERATION.operationCodeStr_.c_str()),
      message_object.GetAllocator());
  message_payload.AddMember("sdp", sdp_value, message_object.GetAllocator());
  message_object.AddMember("payload", message_payload, message_object.GetAllocator());

  rapidjson::StringBuffer strbuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  bool done = message_object.Accept(writer);
  if (!done) {
    LOG(WARNING) << "OnOfferCreated: INVALID JSON!";
    return;
  }
  std::string payload = strbuf.GetString();

  // description contains information about media capabilities
  // (for example, if it has a webcam or can play audio).
  // rtcPeerConnection.setLocalDescription(description);
  // webSocketConnection.send(JSON.stringify({type: WS_OFFER_OPCODE, payload: description}));

  if (wsSess && wsSess.get() && wsSess->isOpen())
    wsSess->send(payload); //*/

  // client sends IceCandidate
}

static void offerCallback(std::shared_ptr<gloer::net::SessionPair> clientSession, NetworkManager* nm,
                          std::shared_ptr<std::string> messageBuffer) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WS: type == offer";

  LOG(WARNING) << "offerCallback on client!!!!";
}

static void answerCallback(std::shared_ptr<gloer::net::SessionPair> clientSession, NetworkManager* nm,
                           std::shared_ptr<std::string> messageBuffer) {
  if (!messageBuffer || !messageBuffer.get() || messageBuffer->empty()) {
    LOG(WARNING) << "WsServer: Invalid messageBuffer";
    return;
  }

  if (!clientSession || !clientSession.get()) {
    LOG(WARNING) << "WSServer invalid clientSession!";
    return;
  }

  if (!nm) {
    LOG(WARNING) << "WSServer invalid NetworkManager!";
    return;
  }

  std::string dataCopy = *messageBuffer.get();

  // const std::string incomingStr = beast::buffers_to_string(messageBuffer->data());
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "answerCallback incomingMsg=" << dataCopy;
  // send same message back (ping-pong)
  // clientSession->send(incomingStr);

  rapidjson::Document message_obj;     // TODO
  message_obj.Parse(dataCopy.c_str()); // TODO

  const auto sdp = WRTCServer::sessionDescriptionStrFromJson(message_obj);

  std::shared_ptr<WRTCSession> createdWRTCSession = clientSession->getWRTCSession().lock();
  if (!createdWRTCSession) {
    LOG(WARNING) << "answerCallback: empty createdWRTCSession!";
    return;
  }

  createdWRTCSession->updateDataChannelState();

  // RTC_DCHECK(createdWRTCSession->isDataChannelOpen() == true);

  // TODO:
  // github.com/YOU-i-Labs/webkit/blob/master/Source/WebCore/Modules/mediastream/libwebrtc/LibWebRTCMediaEndpoint.cpp#L182
  auto clientSessionDescription = createdWRTCSession->createSessionDescription(kAnswer, sdp);
  if (!clientSessionDescription) {
    LOG(WARNING) << "empty clientSessionDescription!";
    return;
  }

  // TODO
  // std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  // TODO: CreateSessionDescriptionMsg : public rtc::MessageData
  // https://github.com/modulesio/webrtc/blob/master/pc/webrtcsessiondescriptionfactory.cc#L68

  createdWRTCSession->setRemoteDescription(clientSessionDescription);

  ///////////createdWRTCSession->CreateAnswer();

  // TODO
  // std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // wait isDataChannelOpen() ==
  // true

  RTC_DCHECK(createdWRTCSession->isExpired() == false);
  RTC_DCHECK(createdWRTCSession->IsStable() == true);
  RTC_DCHECK(createdWRTCSession->isClosing() == false);
  ////////RTC_DCHECK(createdWRTCSession->isDataChannelOpen() == true);

  LOG(WARNING) << "TODO: answerCallback"; ///// <<<<<<<<<<<<<<<<<<<<<<<,
}

} // namespace

using namespace std::chrono_literals;

using namespace ::gloer::net::ws;
using namespace ::gloer::net::wrtc;
using namespace ::gloer::algo;

using namespace ::gameclient;

namespace {
// folly::Singleton<GameServer> gGame;
}

static void printNumOfCores() {
  unsigned int c = std::thread::hardware_concurrency();
  LOG(INFO) << "Number of cores: " << c;
  const size_t minCores = 4;
  if (c < minCores) {
    LOG(INFO) << "Too low number of cores! Prefer servers with at least " << minCores << " cores";
  }
}

/*class ScopedIOC {
public:
  ScopedIOC(const int thread_num) : ioc_(thread_num) {}

  boost::asio::io_context& getIOC() { return ioc_; }

private:
  // The io_context is required for all I/O
  boost::asio::io_context ioc_;
};*/

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

int main(int argc, char* argv[]) {
  // folly::init(&argc, &argv, /* remove recognized gflags */ true);
  using namespace ::gloer::net::ws;
  using namespace ::gloer::net::wrtc;
  using namespace ::gloer::algo;

  std::locale::global(std::locale::classic()); // https://stackoverflow.com/a/18981514/10904212

  size_t WRTCTickFreq = 100; // 1/Freq
  size_t WRTCTickNum = 0;

  size_t WSTickFreq = 100; // 1/Freq
  size_t WSTickNum = 0;

  gloer::log::Logger lg; // inits Logger
  LOG(INFO) << "created Logger...";

  LOG(INFO) << "Starting client..";

  // std::weak_ptr<GameServer> gameInstance = folly::Singleton<GameServer>::try_get();
  // gameInstance->init(gameInstance);

  std::shared_ptr<GameClient> gameInstance = std::make_shared<GameClient>();
  gameInstance->init(gameInstance);

  printNumOfCores();

  uint32_t sendCounter = 0;

  const std::string clientNickname = gloer::algo::genGuid();

  const ::fs::path workdir = gloer::storage::getThisBinaryDirectoryPath();

  // TODO: support async file read, use futures or std::async
  // NOTE: future/promise Should Not Be Coupled to std::thread Execution Agents
  gloer::config::ServerConfig serverConfig(
      ::fs::path{/*workdir / gloer::config::ASSETS_DIR / gloer::config::CONFIGS_DIR /
                 gloer::config::CONFIG_NAME*/},
      workdir);

  // NOTE: Tell the socket to bind to port 0 - random port
  // serverConfig.wsPort_ = static_cast<unsigned short>(0);
  serverConfig.wsPort_ = static_cast<unsigned short>(8085);
  const int thread_num = 1;
  serverConfig.threads_ = thread_num;

  LOG(INFO) << "make_shared NetworkManager...";
  gameInstance->nm = std::make_shared<::gloer::net::NetworkManager>(serverConfig);

  // TODO: print active sessions

  // TODO: destroy inactive wrtc sessions (by timer)

  LOG(INFO) << "runAsClient...";

  gameInstance->nm->runAsClient(serverConfig);

  using namespace gloer;
  const WsNetworkOperation PING_OPERATION =
      WsNetworkOperation(algo::WS_OPCODE::PING, algo::Opcodes::opcodeToStr(algo::WS_OPCODE::PING));
  gameInstance->nm->getWS()->addCallback(PING_OPERATION, &pingCallback);

  const WsNetworkOperation CANDIDATE_OPERATION = WsNetworkOperation(
      algo::WS_OPCODE::CANDIDATE, algo::Opcodes::opcodeToStr(algo::WS_OPCODE::CANDIDATE));
  gameInstance->nm->getWS()->addCallback(CANDIDATE_OPERATION, &candidateCallback);

  const WsNetworkOperation OFFER_OPERATION = WsNetworkOperation(
      algo::WS_OPCODE::OFFER, algo::Opcodes::opcodeToStr(algo::WS_OPCODE::OFFER));
  gameInstance->nm->getWS()->addCallback(OFFER_OPERATION, &offerCallback);

  const WsNetworkOperation ANSWER_OPERATION = WsNetworkOperation(
      algo::WS_OPCODE::ANSWER, algo::Opcodes::opcodeToStr(algo::WS_OPCODE::ANSWER));
  gameInstance->nm->getWS()->addCallback(ANSWER_OPERATION, &answerCallback);

  LOG(INFO) << "Set getWS()->SetOnNewSessionHandler...";

  gameInstance->nm->getWS_SM().SetOnNewSessionHandler(
      [&gameInstance](std::shared_ptr<gloer::net::SessionPair> sess) {
        sess->SetOnMessageHandler(std::bind(&WSServerManager::handleIncomingJSON,
                                            gameInstance->wsGameManager, std::placeholders::_1,
                                            std::placeholders::_2));
        sess->SetOnCloseHandler(std::bind(&WSServerManager::handleClose,
                                          gameInstance->wsGameManager, std::placeholders::_1));
      });

  LOG(INFO) << "Set getWRTC()->SetOnNewSessionHandler...";

  gameInstance->nm->getWRTC_SM().SetOnNewSessionHandler(
      [&gameInstance](std::shared_ptr<WRTCSession> sess) {
        sess->SetOnMessageHandler(std::bind(&WRTCServerManager::handleIncomingJSON,
                                            gameInstance->wrtcGameManager, std::placeholders::_1,
                                            std::placeholders::_2));
        sess->SetOnCloseHandler(std::bind(&WRTCServerManager::handleClose,
                                          gameInstance->wrtcGameManager, std::placeholders::_1));
      });

  LOG(INFO) << "Starting client loop for event queue";

  // processRecievedMsgs
  TickManager<std::chrono::milliseconds> tm(50ms);

  // Create the session and run it
  const auto newSessId = "@clientSideServerId@";
  ::boost::asio::ssl::context ctx_{::boost::asio::ssl::context::tlsv12};

  //auto newWsSession = gameInstance->nm->getWS()->addClientSession(newSessId);

  //::boost::asio::io_context ioc(thread_num);

  /*gloer::net::ws::Client scopedIOC
   = (gameInstance->nm.get(), serverConfig);*/

  auto newWsSession = std::make_shared<ClientSession>(
    //gameInstance->nm->getWS()->getIOC(),
    //scopedIOC.getIOC(),
    gameInstance->nm->getWSClient()->getIOC(),
    //ioc,
    ctx_,
    gameInstance->nm.get(),
    newSessId);

  //gameInstance->nm->getWS()->addSession(newSessId, newWsSession);
  //scopedIOC.addSession(newSessId, newWsSession);
  gameInstance->nm->getWS_SM().addSession(newSessId, newWsSession);

  /*auto newWsSession = std::make_shared<ClientSession>(
    //std::move(socket),
    io,
    ctx_, gameInstance->nm, newSessId);
  gameInstance->nm->getWS()->addSession(newSessId, newWsSession);*/

  if (!gameInstance->nm->getWS_SM().onNewSessCallback_) {
    LOG(WARNING) << "WS: Not set onNewSessCallback_!";
    return EXIT_FAILURE;
  }

  gameInstance->nm->getWS_SM().onNewSessCallback_(newWsSession);

  if (!newWsSession || !newWsSession.get()) {
    LOG(WARNING) << "addClientSession failed ";
    gameInstance->nm->finishServers();
    LOG(WARNING) << "exiting...";
    return EXIT_SUCCESS;
  }
  newWsSession->SetOnMessageHandler(std::bind(&WSServerManager::handleIncomingJSON,
                                              gameInstance->wsGameManager, std::placeholders::_1,
                                              std::placeholders::_2));
  newWsSession->SetOnCloseHandler(
      std::bind(&WSServerManager::handleClose, gameInstance->wsGameManager, std::placeholders::_1));

  /// TODO: post task in main thread sequence
#if 0 // TODO <<<<<<<
  newWsSession->setCreatedCb([&gameInstance, &newWsSession](const std::string& state){
    LOG(WARNING) << "newWsSession createdCb!";
    RTC_DCHECK(newWsSession);
    auto newWRTCSession =
        WRTCServer::setRemoteDescriptionAndCreateOffer(newWsSession, gameInstance->nm.get());
    if (!newWRTCSession.get()) {
      LOG(WARNING) << "can`t create WRTCSession...";
      return;
    }
    newWRTCSession->SetOnMessageHandler(std::bind(&WRTCServerManager::handleIncomingJSON,
                                                  gameInstance->wrtcGameManager,
                                                  std::placeholders::_1, std::placeholders::_2));
    newWRTCSession->SetOnCloseHandler(std::bind(
        &WRTCServerManager::handleClose, gameInstance->wrtcGameManager, std::placeholders::_1));
  });
#else
  newWsSession->setCreatedCb([&gameInstance, &newWsSession](const std::string& state){
    LOG(WARNING) << "created Cb in" << state;
  });
#endif

  const std::string hostToConnect = "localhost";
  const std::string portToConnect = "8085";
  newWsSession->connectAsClient(hostToConnect, portToConnect);

  /// \note need to wait for connected state before starting to use webrtc
  LOG(WARNING) << "connecting to " << hostToConnect << ":" << portToConnect << "...";

  /*LOG(WARNING) << "newWsSession->runAsClient...";
  newWsSession->runAsClient();*/

#if 0
  bool isConnected = newWsSession->waitForConnect(/* maxWait_ms */ 5000);
  if (!isConnected) {
    LOG(WARNING) << "waitForConnect: Can`t connect to " << hostToConnect << ":" << portToConnect;
    gameInstance->nm->finishServers();
    LOG(WARNING) << "exiting...";
    return EXIT_SUCCESS;
  }
#endif // 0

  // TODO
  // std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  // TODO
  // std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  tm.addTickHandler(
      TickHandler("handleAllPlayerMessages", [&gameInstance, &newSessId, &newWsSession]() {
        // Handle queued incoming messages
        gameInstance->handleIncomingMessages();
      }));

  tm.addTickHandler(TickHandler("sendTestMessage", [&gameInstance, &sendCounter, &newWsSession,
                                                    /*&newWRTCSession,*/ &clientNickname]() {
    const std::string msg =
        createTestMessage(std::to_string(sendCounter++) + " message from " + clientNickname);
    /// newWsSession->send(msg);
    /// newWRTCSession->send(msg);
    gameInstance->nm->getWRTC()->sendToAll("12313123123123123");
  }));

  // Run the I/O service on the requested number of threads
  std::vector<std::thread> wsThreads_;

  // Run the I/O service. The call will return when
  // the socket is closed.
  wsThreads_.reserve(thread_num);
  for (auto i = thread_num; i > 0; --i) {
    wsThreads_.emplace_back([&gameInstance] {
      gameInstance->nm->getWSClient()->getIOC().run();
    });
    /*wsThreads_.emplace_back([&scopedIOC] {
      scopedIOC.getIOC().run();
    });*/
    /*wsThreads_.emplace_back([&gameInstance] {
      gameInstance->nm->getWS()->getIOC().run();
    });*/
    /*wsThreads_.emplace_back([&ioc] {
      ioc.run();
    });*/
  }

  while (tm.needServerRun()) {
    tm.tick();
  }

  // TODO: sendProcessedMsgs in separate thread

  // (If we get here, it means we got a SIGINT or SIGTERM)
  LOG(WARNING) << "If we get here, it means we got a SIGINT or SIGTERM";

  gameInstance->nm->finishServers();

  // folly::SingletonVault::singleton()->destroyInstances();
  gameInstance.reset();

  lg.shutDownLogging();

  return EXIT_SUCCESS;
}

#if 0

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------

// Report a failure
void
fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

// Sends a WebSocket message and prints the response
class session : public std::enable_shared_from_this<session>
{
    tcp::resolver resolver_;
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    std::string host_;
    std::string text_;

public:
    // Resolver and socket require an io_context
    explicit
    session(net::io_context& ioc)
        : resolver_(net::make_strand(ioc))
        , ws_(net::make_strand(ioc))
    {
    }

    // Start the asynchronous operation
    void
    run(
        char const* host,
        char const* port,
        char const* text)
    {
        // Save these for later
        host_ = host;
        text_ = text;

        // Look up the domain name
        resolver_.async_resolve(
            host,
            port,
            beast::bind_front_handler(
                &session::on_resolve,
                shared_from_this()));
    }

    void
    on_resolve(
        beast::error_code ec,
        tcp::resolver::results_type results)
    {
        if(ec)
            return fail(ec, "resolve");

        // Set the timeout for the operation
        beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(ws_).async_connect(
            results,
            beast::bind_front_handler(
                &session::on_connect,
                shared_from_this()));
    }

    void
    on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type)
    {
        if(ec)
            return fail(ec, "connect");

        // Turn off the timeout on the tcp_stream, because
        // the websocket stream has its own timeout system.
        beast::get_lowest_layer(ws_).expires_never();

        // Set suggested timeout settings for the websocket
        ws_.set_option(
            websocket::stream_base::timeout::suggested(
                beast::role_type::client));

        // Set a decorator to change the User-Agent of the handshake
        ws_.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req)
            {
                req.set(http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-client-async");
            }));

        // Perform the websocket handshake
        ws_.async_handshake(host_, "/",
            beast::bind_front_handler(
                &session::on_handshake,
                shared_from_this()));
    }

    void
    on_handshake(beast::error_code ec)
    {
        if(ec)
            return fail(ec, "handshake");

        // Send the message
        ws_.async_write(
            net::buffer(text_),
            beast::bind_front_handler(
                &session::on_write,
                shared_from_this()));
    }

    void
    on_write(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "write");

        // Read a message into our buffer
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &session::on_read,
                shared_from_this()));
    }

    void
    on_read(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "read");

        // Close the WebSocket connection
        ws_.async_close(websocket::close_code::normal,
            beast::bind_front_handler(
                &session::on_close,
                shared_from_this()));
    }

    void
    on_close(beast::error_code ec)
    {
        if(ec)
            return fail(ec, "close");

        // If we get here then the connection is closed gracefully

        // The make_printable() function helps print a ConstBufferSequence
        std::cout << beast::make_printable(buffer_.data()) << std::endl;
    }
};

//------------------------------------------------------------------------------

int main(int argc, char** argv)
{
    // The io_context is required for all I/O
    net::io_context ioc;

    // Launch the asynchronous operation
    std::make_shared<session>(ioc)->run("localhost", "8085", "text");

    // Run the I/O service. The call will return when
    // the socket is closed.
    ioc.run();

    return EXIT_SUCCESS;
}
#endif // 0

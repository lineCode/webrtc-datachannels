#include "net/wrtc/WRTCServer.hpp" // IWYU pragma: associated
#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
#include "algo/StringUtils.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include "net/wrtc/Observers.hpp"
#include "net/wrtc/WRTCSession.hpp"
#include "net/ws/WsServer.hpp"
#include "net/ws/WsSession.hpp"
#include <api/call/callfactoryinterface.h>
#include <api/jsep.h>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <logging/rtc_event_log/rtc_event_log_factory_interface.h>
#include <p2p/base/portallocator.h>
#include <rapidjson/encodings.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rtc_base/copyonwritebuffer.h>
#include <rtc_base/scoped_ref_ptr.h>
#include <rtc_base/thread.h>
#include <string>
#include <thread>
#include <webrtc/api/peerconnectioninterface.h>
#include <webrtc/media/base/mediaengine.h>
#include <webrtc/p2p/base/basicpacketsocketfactory.h>
#include <webrtc/p2p/client/basicportallocator.h>
#include <webrtc/rtc_base/bind.h>
#include <webrtc/rtc_base/checks.h>
#include <webrtc/rtc_base/messagehandler.h>
#include <webrtc/rtc_base/messagequeue.h>
#include <webrtc/rtc_base/rtccertificategenerator.h>
#include <webrtc/rtc_base/ssladapter.h>
#include <webrtc/rtc_base/thread.h>

/*
namespace rtc {

MessageHandler::~MessageHandler() { MessageQueueManager::Clear(this); }

} // namespace rtc
*/
namespace gloer {
namespace net {
namespace wrtc {

using namespace gloer::net::ws;

// @see w3c.github.io/webrtc-pc/#rtcsignalingstate-enum
const char kOffer[] = "offer";
const char kPrAnswer[] = "pranswer";
const char kAnswer[] = "answer";

namespace {

// TODO: prevent collision? respond ERROR to client if collided?
static std::string nextWrtcSessionId() { return gloer::algo::genGuid(); }

static void pingCallback(std::shared_ptr<WRTCSession> clientSession, NetworkManager* nm,
                         std::shared_ptr<std::string> messageBuffer) {
  if (!messageBuffer || !messageBuffer.get() || messageBuffer->empty()) {
    LOG(WARNING) << "WRTCServer: Invalid messageBuffer";
    return;
  }

  if (!clientSession || !clientSession.get()) {
    LOG(WARNING) << "WRTCServer invalid clientSession!";
    return;
  }

  if (!nm) {
    LOG(WARNING) << "WRTCServer invalid NetworkManager!";
    return;
  }

  std::string dataCopy = *messageBuffer.get();

  // const std::string incomingStr = beast::buffers_to_string(messageBuffer->data());
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "pingCallback incomingMsg=" << dataCopy;

  // send same message back (ping-pong)
  if (clientSession && clientSession.get() && clientSession->isDataChannelOpen() &&
      !clientSession->isExpired())
    clientSession->send(dataCopy);
}

static void keepaliveCallback(std::shared_ptr<WRTCSession> clientSession, NetworkManager* nm,
                              std::shared_ptr<std::string> messageBuffer) {
  if (!messageBuffer || !messageBuffer.get() || messageBuffer->empty()) {
    LOG(WARNING) << "WRTCServer: Invalid messageBuffer";
    return;
  }

  if (!clientSession || !clientSession.get()) {
    LOG(WARNING) << "WRTCServer invalid clientSession!";
    return;
  }

  if (!nm) {
    LOG(WARNING) << "WRTCServer invalid NetworkManager!";
    return;
  }

  std::string dataCopy = *messageBuffer.get();
}

static void serverTimeCallback(std::shared_ptr<WRTCSession> clientSession, NetworkManager* nm,
                               std::shared_ptr<std::string> messageBuffer) {
  if (!messageBuffer || !messageBuffer.get() || messageBuffer->empty()) {
    LOG(WARNING) << "WRTCServer: Invalid messageBuffer";
    return;
  }

  if (!clientSession || !clientSession.get()) {
    LOG(WARNING) << "WRTCServer invalid clientSession!";
    return;
  }

  if (!nm) {
    LOG(WARNING) << "WRTCServer invalid NetworkManager!";
    return;
  }

  std::string dataCopy = *messageBuffer.get();

  // const std::string incomingStr = beast::buffers_to_string(messageBuffer->data());
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "serverTimeCallback incomingMsg=" << dataCopy;

  std::chrono::system_clock::time_point nowTp = std::chrono::system_clock::now();
  std::time_t t = std::chrono::system_clock::to_time_t(nowTp);

  std::string msg = "serverTimeCallback: ";
  msg += std::ctime(&t);

  // send same message back (ping-pong)
  if (clientSession && clientSession.get() && clientSession->isDataChannelOpen() &&
      !clientSession->isExpired())
    clientSession->send(msg);
}

} // namespace

WRTCInputCallbacks::WRTCInputCallbacks() {}

WRTCInputCallbacks::~WRTCInputCallbacks() {}

std::map<WRTCNetworkOperation, WRTCNetworkOperationCallback>
WRTCInputCallbacks::getCallbacks() const {
  return operationCallbacks_;
}

void WRTCInputCallbacks::addCallback(const WRTCNetworkOperation& op,
                                     const WRTCNetworkOperationCallback& cb) {
  operationCallbacks_[op] = cb;
}

WRTCServer::WRTCServer(NetworkManager* nm, const gloer::config::ServerConfig& serverConfig)
    : nm_(nm), webrtcConf_(webrtc::PeerConnectionInterface::RTCConfiguration()),
      webrtcGamedataOpts_(webrtc::PeerConnectionInterface::RTCOfferAnswerOptions()) {

  // @see
  // webrtc.googlesource.com/src/+/master/examples/objcnativeapi/objc/objc_call_client.mm#63
  // Changes the thread that is checked for in CalledOnValidThread. This may
  // be useful when an object may be created on one thread and then used
  // exclusively on another thread.
  thread_checker_.DetachFromThread();

  /*WRTCQueue_ =
      std::make_shared<algo::DispatchQueue>(std::string{"WebRTC Server Dispatch Queue"}, 0);*/

  // callbacks
  const WRTCNetworkOperation PING_OPERATION = WRTCNetworkOperation(
      algo::WRTC_OPCODE::PING, algo::Opcodes::opcodeToStr(algo::WRTC_OPCODE::PING));
  operationCallbacks_.addCallback(PING_OPERATION, &pingCallback);

  const WRTCNetworkOperation SERVER_TIME_OPERATION = WRTCNetworkOperation(
      algo::WRTC_OPCODE::SERVER_TIME, algo::Opcodes::opcodeToStr(algo::WRTC_OPCODE::SERVER_TIME));
  operationCallbacks_.addCallback(SERVER_TIME_OPERATION, &serverTimeCallback);

  const WRTCNetworkOperation KEEPALIVE_OPERATION = WRTCNetworkOperation(
      algo::WRTC_OPCODE::KEEPALIVE, algo::Opcodes::opcodeToStr(algo::WRTC_OPCODE::KEEPALIVE));
  operationCallbacks_.addCallback(KEEPALIVE_OPERATION, &keepaliveCallback);

  {
    // ICE is the protocol chosen for NAT traversal in WebRTC.
    webrtc::PeerConnectionInterface::IceServer ice_servers[5];
    // TODO to ServerConfig + username/password
    // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    /*ice_servers[0].uri = "stun:stun.l.google.com:19302";
    ice_servers[1].uri = "stun:stun1.l.google.com:19302";
    ice_servers[2].uri = "stun:stun2.l.google.com:19305";
    ice_servers[3].uri = "stun:stun01.sipphone.com";
    ice_servers[4].uri = "stun:stunserver.org";*/
    // TODO ice_server.username = "xxx";
    // TODO ice_server.password = kTurnPassword;
    // TODO
    // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    resetWebRtcConfig_t(
        {} /*{ice_servers[0], ice_servers[1], ice_servers[2], ice_servers[3], ice_servers[4]}*/);
  }
}

WRTCServer::~WRTCServer() { // TODO: virtual
  // auto call Quit()?
}

std::string WRTCServer::sessionDescriptionStrFromJson(const rapidjson::Document& message_object) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "sessionDescriptionStrFromJson";
  webrtc::SdpParseError error;
  std::string sdp = message_object["payload"]["sdp"].GetString();
  return sdp;
}

void WRTCServer::InitAndRun_t() {
  RTC_DCHECK_RUN_ON(&thread_checker_);

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::InitAndRun";
  if (!rtc::InitRandom(static_cast<int>(rtc::Time32()))) {
    LOG(WARNING) << "Error in InitializeSSL()";
  }

  // Create the PeerConnectionFactory.
  if (!rtc::InitializeSSL()) {
    LOG(WARNING) << "Error in InitializeSSL()";
  }

  // @see
  // github.com/pristineio/webrtc-mirror/blob/7a5bcdffaab90a05bc1146b2b1ea71c004e54d71/webrtc/rtc_base/thread.cc
  owned_networkThread_ = networkThreadWithSocketServer_
                             ? rtc::Thread::CreateWithSocketServer()
                             : rtc::Thread::Create(); // reset(new rtc::Thread());
  owned_networkThread_->SetName("network_thread1", nullptr);
  network_thread_ = owned_networkThread_.get();
  // NOTE: check will be executed regardless of compilation mode.
  RTC_CHECK(owned_networkThread_->Start()) << "Failed to start network_thread";
  LOG(INFO) << "Started network_thread";

  owned_workerThread_ = rtc::Thread::Create();
  owned_workerThread_->SetName("worker_thread1", nullptr);
  worker_thread_ = owned_workerThread_.get();
  // NOTE: check will be executed regardless of compilation mode.
  RTC_CHECK(owned_workerThread_->Start()) << "Failed to start worker_thread";
  LOG(INFO) << "Started worker_thread";

  owned_signalingThread_ = rtc::Thread::Create();
  owned_signalingThread_->SetName("signaling_thread1", nullptr);
  signaling_thread_ = owned_signalingThread_.get();
  // NOTE: check will be executed regardless of compilation mode.
  RTC_CHECK(owned_signalingThread_->Start()) << "Failed to start signaling_thread";
  LOG(INFO) << "Started signaling_thread";

  wrtcNetworkManager_.reset(new rtc::BasicNetworkManager());
  socketFactory_.reset(
      new rtc::BasicPacketSocketFactory(owned_networkThread_.get())); // or _workerThread?

  const bool hasPCI = owned_workerThread_->Invoke<bool>(RTC_FROM_HERE, [this]() {
    // @see
    // github.com/sourcey/libsourcey/blob/master/src/webrtc/src/peerfactorycontext.cpp#L53
    peerConnectionFactory_ = webrtc::CreateModularPeerConnectionFactory(
        owned_networkThread_.get(), owned_workerThread_.get(), owned_signalingThread_.get(),
        nullptr, nullptr, nullptr);

    // TODO: pass other settings e.t.c.
    // @see
    // github.com/sourcey/libsourcey/blob/master/src/webrtc/src/peerfactorycontext.cpp#L53
    /*peerConnectionFactory_ = webrtc::CreateModularPeerConnectionFactory(nullptr, nullptr, nullptr,
                                                                      nullptr, nullptr, nullptr);*/

    LOG(INFO) << "Created PeerConnectionFactory";
    if (peerConnectionFactory_.get() == nullptr) {
      LOG(WARNING) << "Error: Could not create CreatePeerConnectionFactory.";
      return false;
    }

    return true;
  });

  if (!hasPCI) {
    LOG(WARNING) << "Error: Could not create CreatePeerConnectionFactory.";
    return;
  }

  /*rtc::Thread* signalingThread = rtc::Thread::Current();
  signalingThread->Run();*/

  /* // TODO
   * while(handleWrtcMsgs) {

  }*/

  /*while (true) {
    // ProcessMessages will process I/O and dispatch messages until:
    //  1) cms milliseconds have elapsed (returns true)
    //  2) Stop() is called (returns false)
    workerThread_->ProcessMessages( 200 ); // cms milliseconds
  }*/

  LOG(WARNING) << "WebRTC starting thread finished";
}

void WRTCServer::resetWebRtcConfig_t(
    const std::vector<webrtc::PeerConnectionInterface::IceServer>& iceServers) {
  RTC_DCHECK_RUN_ON(&thread_checker_);

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::resetWebRtcConfig";

  // settings for game-server messaging
  webrtcGamedataOpts_.offer_to_receive_audio = false;
  webrtcGamedataOpts_.offer_to_receive_video = false;

  // @see w3.org/TR/webrtc/#idl-def-rtcdatachannelinit
  // SCTP supports unordered data.
  // Unordered data is unimportant for multiplayer games.
  // If ordered set to false, data is allowed to be delivered out of order.
  dataChannelConf_.ordered = false;
  // reliable Deprecated. Reliability is assumed, and channel will be unreliable if
  // maxRetransmitTime or MaxRetransmits is set.
  // dataChannelConf_.reliable = false;

  // maxRetransmits is 0, because if a message didn’t arrive, we don’t care.
  dataChannelConf_.maxRetransmits = 0;

  // True if the channel has been externally negotiated and we do not send an
  // in-band signalling in the form of an "open" message. If this is true, |id|
  // below must be set; otherwise it should be unset and will be negotiated
  // in-band.
  dataChannelConf_.negotiated = false;

  // The stream id, or SID, for SCTP data channels. -1 if unset (see above).
  dataChannelConf_.id = -1;

  // TODO: more webrtcConf_ settings
  // github.com/WebKit/webkit/blob/master/Source/ThirdParty/libwebrtc/Source/webrtc/pc/peerconnection.cc#L3117

  // @see w3c.github.io/webrtc-pc/#rtcconfiguration-dictionary
  // set servers
  webrtcConf_.servers.clear();
  for (const auto& ice_server : iceServers) {
    // ICE is the protocol chosen for NAT traversal in WebRTC.
    // @see
    // chromium.googlesource.com/external/webrtc/+/lkgr/pc/peerconnection.cc
    LOG(INFO) << "added ice_server " << ice_server.uri;
    webrtcConf_.servers.push_back(ice_server);
    // If set to true, use RTP data channels instead of SCTP.
    // TODO(deadbeef): Remove this. We no longer commit to supporting RTP data
    // channels, though some applications are still working on moving off of
    // them.
    // RTP data channel rate limited!
    // richard.to/programming/sending-images-with-webrtc-data-channels.html
    // webrtcConfiguration.enable_rtp_data_channel = true;
    // webrtcConfiguration.enable_rtp_data_channel = false;
    // Can be used to disable DTLS-SRTP. This should never be done, but can be
    // useful for testing purposes, for example in setting up a loopback call
    // with a single PeerConnection.
    // webrtcConfiguration.enable_dtls_srtp = false;
    // webrtcConfiguration.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
    // webrtcConfiguration.DtlsSrtpKeyAgreement
  }
}

void WRTCServer::finishThreads_t() {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::Quit";

  RTC_DCHECK_RUN_ON(&thread_checker_);

  /*{
    if (!nm_->getWRTC()->workerThread_ || !nm_->getWRTC()->workerThread_.get()) {
      LOG(WARNING) << "WRTCSession::finishThreads: invalid workerThread_";
      return;
    }

    if (!nm_->getWRTC()->workerThread_->IsCurrent()) {
      return nm_->getWRTC()->workerThread_->Invoke<void>(RTC_FROM_HERE,
                                                         [this] { return finishThreads(); });
    }
  }*/

  doToAllSessions([&](const std::string& sessId, std::shared_ptr<WRTCSession> session) {
    unregisterSession(sessId);
    if (session) {
      // session->close_s(false); // called from unregisterSession
      // session.reset();
      // session = nullptr;
    }
  });
  /*LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::Quit2";*/

  while (getSessionsCount() != 0) {
    LOG(INFO) << std::this_thread::get_id() << ":"
              << "getSessionsCount() != 0, sleep";
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  /*LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::Quit3";*/

  // webrtc_thread.reset(); // TODO

  /*LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::Quit4";*/

  {
    // rtc::CritScope lock(&pcMutex_);
    if (peerConnectionFactory_.get() == nullptr) {
      LOG(WARNING) << "Error: Invalid CreatePeerConnectionFactory.";
      return;
    }
    peerConnectionFactory_ = nullptr;
  }

  /*LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::Quit5";*/

  // CloseDataChannel?
  // webrtc.googlesource.com/src/+/master/examples/unityplugin/simple_peer_connection.cc
  // Tells the thread to stop and waits until it is joined.
  // Never call Stop on the current thread.  Instead use the inherited Quit
  // function which will exit the base MessageQueue without terminating the
  // underlying OS thread.
  if (owned_networkThread_.get())
    owned_networkThread_->Quit();
  if (owned_signalingThread_.get())
    owned_signalingThread_->Quit();
  if (owned_workerThread_.get())
    owned_workerThread_->Quit();

  /*LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::Quit6";*/

  rtc::CleanupSSL();

  // webrtcStartThread_.join();

  /*LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::Quit7";*/
}

/*rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> WRTCServer::getPCF() const {
  return peerConnectionFactory_;
}*/

webrtc::PeerConnectionInterface::RTCConfiguration WRTCServer::getWRTCConf() const {
  return webrtcConf_;
}

void WRTCServer::addGlobalDataChannelCount_s(uint32_t count) {
  RTC_DCHECK_RUN_ON(signaling_thread());

  // LOG(INFO) << "WRTCServer::addGlobalDataChannelCount_s";
  dataChannelGlobalCount_ += count;
  // TODO: check overflow
  LOG(INFO) << "WRTCServer::addGlobalDataChannelCount_s: data channel count: "
            << dataChannelGlobalCount_;

  // expected: 1 session == 1 dataChannel
  // NOTE: reconnect == new session
  RTC_DCHECK_LE(dataChannelGlobalCount_, std::numeric_limits<uint32_t>::max());
  // session may exist with closed dataChannel
  // session may be deleted from map, but still exist
  // RTC_DCHECK_LE(dataChannelGlobalCount_, getSessionsCount());
}

void WRTCServer::subGlobalDataChannelCount_s(uint32_t count) {
  RTC_DCHECK_RUN_ON(signaling_thread());

  // LOG(INFO) << "WRTCServer::subGlobalDataChannelCount_s";
  RTC_DCHECK_GT(dataChannelGlobalCount_, 0);
  dataChannelGlobalCount_ -= count;
  // TODO: check overflow
  LOG(INFO) << "WRTCServer::subGlobalDataChannelCount_s: data channel count: "
            << dataChannelGlobalCount_;

  // expected: 1 session == 1 dataChannel
  // NOTE: reconnect == new session
  RTC_DCHECK_LE(dataChannelGlobalCount_, std::numeric_limits<uint32_t>::max());
  // session may exist with closed dataChannel
  // session may be deleted from map, but still exist
  // RTC_DCHECK_LE(dataChannelGlobalCount_, getSessionsCount());
}

/**
 * @example:
 * std::time_t t = std::chrono::system_clock::to_time_t(p);
 * std::string msg = "server_time: ";
 * msg += std::ctime(&t);
 * sm->sendToAll(msg);
 **/
void WRTCServer::sendToAll(const std::string& message) {
  LOG(WARNING) << "WRTCServer::sendToAll:" << message;
  {
    // NOTE: don`t call getSessions == lock in loop
    const auto sessionsCopy = getSessions();

    for (auto& sessionkv : sessionsCopy) {
      if (!sessionkv.second || !sessionkv.second.get()) {
        LOG(WARNING) << "WRTCServer::sendTo: Invalid session ";
        continue;
      }
      auto session = sessionkv.second;
      if (session && session.get()) {
        session->send(message);
      }
    }
  }
}

void WRTCServer::sendTo(const std::string& sessionID, const std::string& message) {
  {
    // NOTE: don`t call getSessions == lock in loop
    const auto sessionsCopy = getSessions();

    auto it = sessionsCopy.find(sessionID);
    if (it != sessionsCopy.end()) {
      if (!it->second || !it->second.get()) {
        LOG(WARNING) << "WRTCServer::sendTo: Invalid session ";
        return;
      }
      it->second->send(message);
    }
  }
}

/**
 * @brief removes session from list of valid sessions
 *
 * @param id id of session to be removed
 */
void WRTCServer::unregisterSession(const std::string& id) {
  if (!signaling_thread()->IsCurrent()) {
    return signaling_thread()->Invoke<void>(RTC_FROM_HERE,
                                            [this, id] { return unregisterSession(id); });
  }

  RTC_DCHECK_RUN_ON(signaling_thread());

  const std::string idCopy = id; // TODO: unknown lifetime, use idCopy
  std::shared_ptr<WRTCSession> sess = getSessById(idCopy);

  // note: close before removeSessById to keep dataChannelCount <= sessionsCount
  // close datachannel, pci, e.t.c.
  if (sess && sess.get()) {
    if (!signaling_thread()->IsCurrent()) {
      signaling_thread()->Invoke<void>(RTC_FROM_HERE,
                                       [sess] { return sess->close_s(false, false); });
    } else {
      sess->close_s(false, false);
    }
  }

  {
    if (!removeSessById(idCopy)) {
      // throw std::runtime_error(
      // LOG(WARNING) << "WRTCServer::unregisterSession: trying to unregister non-existing session "
      //             << idCopy;
      // NOTE: continue cleanup with saved shared_ptr
    }
    // LOG(WARNING) << "WrtcServer: unregistered " << idCopy;
    /*if (!sess || !sess.get()) {
      LOG(WARNING) << "WRTCServer::unregisterSession: session already deleted";
      return;
    }*/
  }

  // LOG(WARNING) << "WsServer: unregistered " << idCopy;
}

void WRTCServer::runThreads_t(const gloer::config::ServerConfig& serverConfig) {
  RTC_DCHECK_RUN_ON(&thread_checker_);

  // webrtcStartThread_ = std::thread(&WRTCServer::webRtcSignalThreadEntry, this);
  webRtcBackgroundThreadEntry();
}

// The thread entry point for the WebRTC thread. This sets the WebRTC thread as
// the signaling thread and creates a worker thread in the background.
void WRTCServer::webRtcBackgroundThreadEntry() { InitAndRun_t(); }

void WRTCServer::setRemoteDescriptionAndCreateAnswer(std::shared_ptr<WsSession> clientWsSession,
                                                     NetworkManager* nm, const std::string& sdp) {
  // TODO: don`t run heavy operations on signaling_thread!!!
  {
    if (!nm->getWRTC()->signaling_thread()->IsCurrent()) {
      return nm->getWRTC()->signaling_thread()->Invoke<void>(
          RTC_FROM_HERE, [clientWsSession, nm, sdp] {
            return setRemoteDescriptionAndCreateAnswer(clientWsSession, nm, sdp);
          });
    }
  }
  RTC_DCHECK_RUN_ON(nm->getWRTC()->signaling_thread());

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::SetRemoteDescriptionAndCreateAnswer";

  std::shared_ptr<WRTCSession> createdWRTCSession;

  if (!nm || nm == nullptr) { //// <<<<
    LOG(WARNING) << "WsServer: Invalid NetworkManager";
    return;
  }

  if (!clientWsSession || clientWsSession == nullptr) { //// <<<<
    LOG(WARNING) << "WSServer invalid clientSession!";
    return;
  }

  // rtc::scoped_refptr<webrtc::PeerConnectionInterface> newPeerConn;

  const std::string webrtcConnId = nextWrtcSessionId();
  const std::string wsConnId = clientWsSession->getId(); // remember id before session deletion

  // std::unique_ptr<cricket::PortAllocator> port_allocator(new
  // cricket::BasicPortAllocator(new rtc::BasicNetworkManager()));
  // port_allocator->SetPortRange(60000, 60001);
  // TODO ice_server.username = "xxx";
  // TODO ice_server.password = kTurnPassword;
  LOG(INFO) << "creating peer_connection...";
  {
    if (!nm->getWRTC()->wrtcNetworkManager_.get() || !nm->getWRTC()->socketFactory_.get()) {
      LOG(WARNING) << "WRTCServer::setRemoteDescriptionAndCreateAnswer: invalid "
                      "wrtcNetworkManager_ or socketFactory_";
      return;
    }

    LOG(INFO) << "creating WRTCSession...";
    createdWRTCSession = std::make_shared<WRTCSession>(nm, webrtcConnId, wsConnId);

    {
      if (!nm->getWRTC()->onNewSessCallback_) {
        LOG(WARNING) << "WRTC: Not set onNewSessCallback_!";
        return;
      }
      nm->getWRTC()->onNewSessCallback_(createdWRTCSession);
    }

    LOG(INFO) << "creating peerConnectionObserver...";
    createdWRTCSession->createPeerConnectionObserver();

    LOG(WARNING) << "WRTCServer: createPeerConnection...";
    createdWRTCSession->createPeerConnection(); // requires peerConnectionObserver_

    LOG(INFO) << "addSession WRTCSession...";
    {
      auto isSessCreated = nm->getWRTC()->addSession(webrtcConnId, createdWRTCSession);
      if (!isSessCreated) {
        LOG(WARNING) << "setRemoteDescriptionAndCreateAnswer: Can`t create session ";
        return;
      }
      clientWsSession->pairToWRTCSession(createdWRTCSession); // TODO: use Task queue
      LOG(INFO) << "updating peerConnections_ for webrtcConnId = " << webrtcConnId;
    }
  }

  createdWRTCSession->createDCI();

  createdWRTCSession->setObservers();

  createdWRTCSession->updateDataChannelState();

  // TODO:
  // github.com/YOU-i-Labs/webkit/blob/master/Source/WebCore/Modules/mediastream/libwebrtc/LibWebRTCMediaEndpoint.cpp#L182
  webrtc::SessionDescriptionInterface* clientSessionDescription =
      createdWRTCSession->createSessionDescription(kOffer, sdp);
  if (!clientSessionDescription) {
    LOG(WARNING) << "empty clientSessionDescription!";
    return;
  }

  createdWRTCSession->SetRemoteDescription(clientSessionDescription);

  createdWRTCSession->CreateAnswer();
}

// see
// github.com/WebKit/webkit/blob/master/Source/ThirdParty/libwebrtc/Source/webrtc/pc/peerconnectionfactory.h
rtc::Thread* WRTCServer::signaling_thread() {
  // This method can be called on a different thread when the factory is
  // created in CreatePeerConnectionFactory().
  return signaling_thread_;
}

rtc::Thread* WRTCServer::worker_thread() { return worker_thread_; }

rtc::Thread* WRTCServer::network_thread() { return network_thread_; }

} // namespace wrtc
} // namespace net
} // namespace gloer

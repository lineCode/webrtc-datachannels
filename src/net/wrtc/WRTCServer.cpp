#include "net/wrtc/WRTCServer.hpp" // IWYU pragma: associated
#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
#include "algo/StringUtils.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include "net/wrtc/Observers.hpp"
#include "net/wrtc/WRTCSession.hpp"
#include "net/wrtc/wrtc.hpp"
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
#include <string>
#include <thread>
#include <webrtc/api/peerconnectioninterface.h>
#include <webrtc/base/compiler_specific.h>
#include <webrtc/base/macros.h>
#include <webrtc/media/base/mediaengine.h>
#include <webrtc/p2p/base/basicpacketsocketfactory.h>
#include <webrtc/p2p/client/basicportallocator.h>
#include <webrtc/pc/peerconnectionfactory.h>
#include <webrtc/rtc_base/asyncinvoker.h>
#include <webrtc/rtc_base/bind.h>
#include <webrtc/rtc_base/checks.h>
#include <webrtc/rtc_base/copyonwritebuffer.h>
#include <webrtc/rtc_base/messagehandler.h>
#include <webrtc/rtc_base/messagequeue.h>
#include <webrtc/rtc_base/rtccertificategenerator.h>
#include <webrtc/rtc_base/scoped_ref_ptr.h>
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

namespace {
// TODO: prevent collision? respond ERROR to client if collided?
static std::string nextWrtcSessionId() { return ::gloer::algo::genGuid(); }

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

  // std::string dataCopy = *messageBuffer.get();
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

// call from main thread
WRTCServer::WRTCServer(NetworkManager* nm, const gloer::config::ServerConfig& serverConfig)
    : nm_(nm), webrtcConf_(webrtc::PeerConnectionInterface::RTCConfiguration()),
      webrtcGamedataOpts_(webrtc::PeerConnectionInterface::RTCOfferAnswerOptions()) {

  // @see
  // webrtc.googlesource.com/src/+/master/examples/objcnativeapi/objc/objc_call_client.mm#63
  // Changes the thread that is checked for in CalledOnValidThread. This may
  // be useful when an object may be created on one thread and then used
  // exclusively on another thread.
  thread_checker_.DetachFromThread();

  // remember main thread
  startThread_ = rtc::Thread::Current();
  RTC_DCHECK(startThread_ != nullptr);
  if (!startThread_) {
    LOG(WARNING) << "Invalid startThread for WRTCServer";
  }

  asyncInvoker_ = std::make_unique<rtc::AsyncInvoker>();
  /*
 https://github.com/jjzhang166/LibSourcey/blob/ce311ff22ca02c8a83df7162a70f6aa4f760a761/src/webrtc/include/scy/webrtc/yuvvideocapturer.h#L83

 _asyncInvoker->AsyncInvoke<void>(RTC_FROM_HERE, _startThread,
                rtc::Bind(&YuvVideoCapturer::SignalFrameCapturedOnStartThread, this));
*/

  /*WRTCQueue_ =
      std::make_shared<algo::DispatchQueue>(std::string{"WebRTC Server Dispatch Queue"}, 0);*/

  // callbacks
  const WRTCNetworkOperation PING_OPERATION = WRTCNetworkOperation(
      algo::WRTC_OPCODE::PING, algo::Opcodes::opcodeToStr(algo::WRTC_OPCODE::PING));
  addCallback(PING_OPERATION, &pingCallback);

  const WRTCNetworkOperation SERVER_TIME_OPERATION = WRTCNetworkOperation(
      algo::WRTC_OPCODE::SERVER_TIME, algo::Opcodes::opcodeToStr(algo::WRTC_OPCODE::SERVER_TIME));
  addCallback(SERVER_TIME_OPERATION, &serverTimeCallback);

  const WRTCNetworkOperation KEEPALIVE_OPERATION = WRTCNetworkOperation(
      algo::WRTC_OPCODE::KEEPALIVE, algo::Opcodes::opcodeToStr(algo::WRTC_OPCODE::KEEPALIVE));
  addCallback(KEEPALIVE_OPERATION, &keepaliveCallback);

  {
    // TODO dynamic AddServerConfig
    // https://github.com/RainwayApp/spitfire/blob/master/Spitfire/RtcConductor.cpp#L163

    // ICE is the protocol chosen for NAT traversal in WebRTC.
    webrtc::PeerConnectionInterface::IceServer ice_servers[5];
    // TODO to ServerConfig + username/password
    // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    ice_servers[0].uri = "stun:stun.l.google.com:19302";
    ice_servers[1].uri = "stun:stun1.l.google.com:19302";
    ice_servers[2].uri = "stun:stun2.l.google.com:19305";
    ice_servers[3].uri = "stun:stun01.sipphone.com";
    ice_servers[4].uri = "stun:stunserver.org";
    // TODO ice_server.username = "xxx";
    // TODO ice_server.password = kTurnPassword;
    // TODO
    // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    resetWebRtcConfig_t(
        {ice_servers[0], ice_servers[1], ice_servers[2], ice_servers[3], ice_servers[4]});
  }
}

void WRTCServer::addCallback(const WRTCNetworkOperation& op,
                             const WRTCNetworkOperationCallback& cb) {
  operationCallbacks_.addCallback(op, cb);
}

WRTCServer::~WRTCServer() { // TODO: virtual
  // auto call Quit()?
}

std::string WRTCServer::sessionDescriptionStrFromJson(const rapidjson::Document& message_object) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "sessionDescriptionStrFromJson";

  std::string sdp = message_object["payload"]["sdp"].GetString();
  LOG(WARNING) << "sdp = " << sdp;
  return sdp;
}

void WRTCServer::InitAndRun_t() {
  RTC_DCHECK_RUN_ON(&thread_checker_);

  RTC_DCHECK(!peerConnectionFactory_.get());
  RTC_DCHECK(!signaling_thread_);
  RTC_DCHECK(!worker_thread_);
  RTC_DCHECK(!network_thread_);
  RTC_DCHECK(!socketFactory_.get());

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::InitAndRun";
  if (!rtc::InitRandom(static_cast<int>(rtc::Time32()))) {
    LOG(WARNING) << "Error in InitializeSSL()";
  }
  // NOTE: Call this on the main thread, before using SSL.
  // Call CleanupSSL when finished with SSL.
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
  /*if (!network_thread_->IsCurrent()) {
    // @see github.com/modulesio/webrtc/blob/master/pc/channelmanager.cc#L132
    LOG(INFO) << "Running network_thread with SetAllowBlockingCalls = false";
    // Do not allow invoking calls to other threads on the network thread.
    network_thread_->Invoke<void>(RTC_FROM_HERE,
                                  [&] { network_thread_->SetAllowBlockingCalls(false); });
  }*/
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

  socketFactory_.reset(new rtc::BasicPacketSocketFactory(owned_networkThread_.get()));

  const bool hasPCI = owned_workerThread_->Invoke<bool>(RTC_FROM_HERE, [this]() {
    rtc::CritScope lock(&pcfMutex_);
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
  // dataChannelConf_.negotiated = false;

  // The stream id, or SID, for SCTP data channels. -1 if unset (see above).
  // dataChannelConf_.id = -1;

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

    // Can be used to disable DTLS-SRTP. This should never be done, but can be
    // useful for testing purposes, for example in setting up a loopback call
    // with a single PeerConnection.
    // webrtcConfiguration.enable_dtls_srtp = false;

    // webrtcConfiguration.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;

    // webrtcConfiguration.DtlsSrtpKeyAgreement

    webrtcConf_.disable_ipv6 = false;

    // If set to true, don't gather IPv6 ICE candidates on Wi-Fi.
    // Only intended to be used on specific devices. Certain phones disable IPv6
    // when the screen is turned off and it would be better to just disable the
    // IPv6 ICE candidates on Wi-Fi in those cases.
    webrtcConf_.disable_ipv6_on_wifi = false;

    // By default, the PeerConnection will use a limited number of IPv6 network
    // interfaces, in order to avoid too many ICE candidate pairs being created
    // and delaying ICE completion.
    //
    // Can be set to INT_MAX to effectively disable the limit.
    // webrtcConf_.max_ipv6_networks = cricket::kDefaultMaxIPv6Networks;

    // Exclude link-local network interfaces
    // from considertaion for gathering ICE candidates.
    webrtcConf_.disable_link_local_networks = false;
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
    rtc::CritScope lock(&pcfMutex_);
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

  // Call to cleanup additional threads
  // NOTE: Call this on the main thread
  rtc::CleanupSSL();

  // webrtcStartThread_.join();

  /*LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::Quit7";*/
}

/*rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> WRTCServer::getPCF() const {
 * rtc::CritScope lock(&pcfMutex_);
  return peerConnectionFactory_;
}*/

webrtc::PeerConnectionInterface::RTCConfiguration WRTCServer::getWRTCConf() const {
  return webrtcConf_;
}

void WRTCServer::addGlobalDataChannelCount_s(uint32_t count) {
  RTC_DCHECK_RUN_ON(signalingThread());

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
  RTC_DCHECK_RUN_ON(signalingThread());

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
  // LOG(WARNING) << "WRTCServer::sendToAll:" << message;
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
  LOG(WARNING) << "unregisterSession for id = " << id;
  if (!signalingThread()->IsCurrent()) {
    return signalingThread()->Invoke<void>(RTC_FROM_HERE,
                                           [this, id] { return unregisterSession(id); });
  }

  RTC_DCHECK_RUN_ON(signalingThread());

  const std::string idCopy = id; // TODO: unknown lifetime, use idCopy
  std::shared_ptr<WRTCSession> sess = getSessById(idCopy);

  // note: close before removeSessById to keep dataChannelCount <= sessionsCount
  // close datachannel, pci, e.t.c.
  if (sess && sess.get()) {
    if (!signalingThread()->IsCurrent()) {
      signalingThread()->Invoke<void>(RTC_FROM_HERE,
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

  // LOG(WARNING) << "WRTCServer: unregistered " << idCopy;
}

void WRTCServer::runThreads_t(const gloer::config::ServerConfig& serverConfig) {
  RTC_DCHECK_RUN_ON(&thread_checker_);

  // webrtcStartThread_ = std::thread(&WRTCServer::webRtcSignalThreadEntry, this);
  webRtcBackgroundThreadEntry();
}

// The thread entry point for the WebRTC thread. This sets the WebRTC thread as
// the signaling thread and creates a worker thread in the background.
void WRTCServer::webRtcBackgroundThreadEntry() { InitAndRun_t(); }

std::shared_ptr<WRTCSession>
WRTCServer::createNewSession(bool isServer, std::shared_ptr<WsSession> clientWsSession,
                             NetworkManager* nm) {
  // TODO: don`t run heavy operations on signaling_thread!!!
  {
    if (!nm->getWRTC()->signalingThread()->IsCurrent()) {
      return nm->getWRTC()->signalingThread()->Invoke<std::shared_ptr<WRTCSession>>(
          RTC_FROM_HERE, [isServer, clientWsSession, nm] {
            return createNewSession(isServer, clientWsSession, nm);
          });
    }
  }
  RTC_DCHECK_RUN_ON(nm->getWRTC()->signalingThread());

  RTC_DCHECK(nm != nullptr);
  if (!nm) {
    LOG(WARNING) << "WRTCServer: Invalid NetworkManager";
    return nullptr;
  }

  RTC_DCHECK(clientWsSession.get() != nullptr);
  if (!clientWsSession || clientWsSession.get() == nullptr) {
    LOG(WARNING) << "WRTCServer invalid clientSession!";
    return nullptr;
  }

  const bool alreadyPaired = clientWsSession->hasPairedWRTCSession();
  // RTC_DCHECK(alreadyPaired == false);
  if (alreadyPaired) {
    LOG(WARNING) << std::this_thread::get_id() << ":"
                 << "WRTCServer::createNewSession alreadyPaired with " << clientWsSession->getId();
    return nullptr;
  }

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::createNewSession";

  std::shared_ptr<WRTCSession> createdWRTCSession;

  // rtc::scoped_refptr<webrtc::PeerConnectionInterface> newPeerConn;

  const std::string webrtcConnId = nextWrtcSessionId();
  const std::string wsConnId = clientWsSession->getId(); // remember id before session deletion

  // std::unique_ptr<cricket::PortAllocator> port_allocator(new
  // cricket::BasicPortAllocator(new rtc::BasicNetworkManager()));
  // port_allocator->SetPortRange(60000, 60001);
  // TODO: warn if max sess count > PortRange
  // TODO ice_server.username = "xxx";
  // TODO ice_server.password = kTurnPassword;
  LOG(INFO) << "creating peer_connection...";
  {
    RTC_DCHECK(nm->getWRTC()->wrtcNetworkManager_.get() != nullptr);
    if (!nm->getWRTC()->wrtcNetworkManager_.get() || !nm->getWRTC()->socketFactory_.get()) {
      LOG(WARNING) << "WRTCServer::createNewSession: invalid "
                      "wrtcNetworkManager_ or socketFactory_";
      return nullptr;
    }

    LOG(INFO) << "creating WRTCSession...";
    createdWRTCSession = std::make_shared<WRTCSession>(nm, webrtcConnId, wsConnId);

    {
      RTC_DCHECK(nm->getWRTC()->onNewSessCallback_ != nullptr);
      if (!nm->getWRTC()->onNewSessCallback_) {
        LOG(WARNING) << "WRTC: Not set onNewSessCallback_!";
        return nullptr;
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
      RTC_DCHECK(isSessCreated == true);
      if (!isSessCreated) {
        LOG(WARNING) << "createNewSession: Can`t create session ";
        createdWRTCSession->close_s(false, false);
        return nullptr;
      }
      clientWsSession->pairToWRTCSession(createdWRTCSession); // TODO: use Task queue
      LOG(INFO) << "updating peerConnections_ for webrtcConnId = " << webrtcConnId;
    }
  }

  RTC_DCHECK(createdWRTCSession->isDataChannelOpen() == false);
  RTC_DCHECK(createdWRTCSession->fullyCreated() == false);

  createdWRTCSession->createDCI();

  createdWRTCSession->setObservers(isServer);

  createdWRTCSession->updateDataChannelState();

  // RTC_DCHECK(createdWRTCSession->isDataChannelOpen() == true);

  return createdWRTCSession;
}

std::shared_ptr<WRTCSession>
WRTCServer::setRemoteDescriptionAndCreateOffer(std::shared_ptr<WsSession> clientWsSession,
                                               NetworkManager* nm) {

  // TODO: don`t run heavy operations on signaling_thread!!!
  {
    if (!nm->getWRTC()->signalingThread()->IsCurrent()) {
      return nm->getWRTC()->signalingThread()->Invoke<std::shared_ptr<WRTCSession>>(
          RTC_FROM_HERE, [clientWsSession, nm] {
            return setRemoteDescriptionAndCreateOffer(clientWsSession, nm);
          });
    }
  }

  RTC_DCHECK_RUN_ON(nm->getWRTC()->signalingThread());

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::setRemoteDescriptionAndCreateOffer";

  if (!nm || nm == nullptr) { //// <<<<
    LOG(WARNING) << "WRTCServer: Invalid NetworkManager";
    return nullptr;
  }

  if (!clientWsSession || clientWsSession == nullptr) { //// <<<<
    LOG(WARNING) << "WRTCServer invalid clientSession!";
    return nullptr;
  }

  if (!nm->getWRTC()->wrtcNetworkManager_.get() || !nm->getWRTC()->socketFactory_.get()) {
    LOG(WARNING) << "WRTCServer::setRemoteDescriptionAndCreateOffer: invalid "
                    "wrtcNetworkManager_ or socketFactory_";
    return nullptr;
  }

  std::shared_ptr<WRTCSession> createdWRTCSession = createNewSession(false, clientWsSession, nm);

  RTC_DCHECK(createdWRTCSession.get() != nullptr);
  if (!createdWRTCSession) {
    LOG(WARNING) << "setRemoteDescriptionAndCreateOffer: created invalid WRTCSession";
    return nullptr;
  }

  LOG(WARNING) << "WRTCServer::setRemoteDescriptionAndCreateOffer: created WRTC session with id = "
               << createdWRTCSession->getId();
  std::shared_ptr<WRTCSession> wrtcSess = nm->getWRTC()->getSessById(createdWRTCSession->getId());
  RTC_DCHECK(wrtcSess.get() != nullptr);

  createdWRTCSession->updateDataChannelState();

  // RTC_DCHECK(createdWRTCSession->isDataChannelOpen() == true);

  // TODO:
  // github.com/YOU-i-Labs/webkit/blob/master/Source/WebCore/Modules/mediastream/libwebrtc/LibWebRTCMediaEndpoint.cpp#L182
  /*webrtc::SessionDescriptionInterface* clientSessionDescription =
      createdWRTCSession->createSessionDescription(kOffer, sdp);
  if (!clientSessionDescription) {
    LOG(WARNING) << "empty clientSessionDescription!";
    return;
  }

  // TODO: CreateSessionDescriptionMsg : public rtc::MessageData
  // https://github.com/modulesio/webrtc/blob/master/pc/webrtcsessiondescriptionfactory.cc#L68

  createdWRTCSession->SetRemoteDescription(clientSessionDescription);*/

  // TODO
  // std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // TODO

  createdWRTCSession->CreateOffer();

  return createdWRTCSession;
}

// get sdp from client by websockets
void WRTCServer::setRemoteDescriptionAndCreateAnswer(std::shared_ptr<WsSession> clientWsSession,
                                                     NetworkManager* nm, const std::string& sdp) {

  // TODO: don`t run heavy operations on signaling_thread!!!
  {
    if (!nm->getWRTC()->signalingThread()->IsCurrent()) {
      return nm->getWRTC()->signalingThread()->Invoke<void>(
          RTC_FROM_HERE, [clientWsSession, nm, sdp] {
            return setRemoteDescriptionAndCreateAnswer(clientWsSession, nm, sdp);
          });
    }
  }

  RTC_DCHECK_RUN_ON(nm->getWRTC()->signalingThread());

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::SetRemoteDescriptionAndCreateAnswer";

  if (!nm || nm == nullptr) { //// <<<<
    LOG(WARNING) << "WRTCServer: Invalid NetworkManager";
    return;
  }

  if (!clientWsSession || clientWsSession == nullptr) { //// <<<<
    LOG(WARNING) << "WRTCServer invalid clientSession!";
    return;
  }

  if (!nm->getWRTC()->wrtcNetworkManager_.get() || !nm->getWRTC()->socketFactory_.get()) {
    LOG(WARNING) << "WRTCServer::setRemoteDescriptionAndCreateAnswer: invalid "
                    "wrtcNetworkManager_ or socketFactory_";
    return;
  }

  std::shared_ptr<WRTCSession> createdWRTCSession = createNewSession(true, clientWsSession, nm);

  // RTC_DCHECK(createdWRTCSession.get() != nullptr); // may be empty
  if (!createdWRTCSession) {
    LOG(WARNING) << "setRemoteDescriptionAndCreateAnswer: created invalid WRTCSession";
    return;
  }

  LOG(WARNING) << "WRTCServer::setRemoteDescriptionAndCreateAnswer: created WRTC session with id = "
               << createdWRTCSession->getId();
  std::shared_ptr<WRTCSession> wrtcSess = nm->getWRTC()->getSessById(createdWRTCSession->getId());
  RTC_DCHECK(wrtcSess.get() != nullptr);

  createdWRTCSession->updateDataChannelState();

  // RTC_DCHECK(createdWRTCSession->isDataChannelOpen() == true);

  // TODO:
  // github.com/YOU-i-Labs/webkit/blob/master/Source/WebCore/Modules/mediastream/libwebrtc/LibWebRTCMediaEndpoint.cpp#L182

  // got offer from client
  auto clientSessionDescription = createdWRTCSession->createSessionDescription(kOffer, sdp);
  if (!clientSessionDescription) {
    LOG(WARNING) << "empty clientSessionDescription!";
    return;
  }

  // TODO: CreateSessionDescriptionMsg : public rtc::MessageData
  // https://github.com/modulesio/webrtc/blob/master/pc/webrtcsessiondescriptionfactory.cc#L68

  createdWRTCSession->setRemoteDescription(clientSessionDescription);

  createdWRTCSession->CreateAnswer();
}

rtc::Thread* WRTCServer::startThread() { return startThread_; }

// see
// github.com/WebKit/webkit/blob/master/Source/ThirdParty/libwebrtc/Source/webrtc/pc/peerconnectionfactory.h
rtc::Thread* WRTCServer::signalingThread() {
  // This method can be called on a different thread when the factory is
  // created in CreatePeerConnectionFactory().
  return signaling_thread_;
}

rtc::Thread* WRTCServer::workerThread() { return worker_thread_; }

rtc::Thread* WRTCServer::networkThread() { return network_thread_; }

} // namespace wrtc
} // namespace net
} // namespace gloer

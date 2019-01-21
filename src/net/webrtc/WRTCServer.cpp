#include "net/webrtc/WRTCServer.hpp" // IWYU pragma: associated
#include "algorithm/DispatchQueue.hpp"
#include "algorithm/NetworkOperation.hpp"
#include "algorithm/StringUtils.hpp"
#include "config/ServerConfig.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include "net/webrtc/Observers.hpp"
#include "net/webrtc/WRTCSession.hpp"
#include "net/websockets/WsServer.hpp"
#include "net/websockets/WsSession.hpp"
#include <api/call/callfactoryinterface.h>
#include <api/jsep.h>
#include <boost/asio.hpp>
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
#include <webrtc/rtc_base/checks.h>
#include <webrtc/rtc_base/rtccertificategenerator.h>
#include <webrtc/rtc_base/ssladapter.h>

namespace utils {
namespace net {

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// TODO: prevent collision? respond ERROR to client if collided?
static std::string nextWrtcSessionId() { return utils::algo::genGuid(); }

webrtc::SessionDescriptionInterface*
createSessionDescriptionFromJson(const rapidjson::Document& message_object) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "createSessionDescriptionFromJson";
  webrtc::SdpParseError error;
  std::string sdp = message_object["payload"]["sdp"].GetString();
  // LOG(INFO) << "sdp =" << sdp;
  // TODO: free memory?
  // TODO: handle error?
  webrtc::SessionDescriptionInterface* sdi = webrtc::CreateSessionDescription("offer", sdp, &error);
  if (sdi == nullptr) {
    LOG(WARNING) << "createSessionDescriptionFromJson:: SDI IS NULL" << error.description.c_str();
    LOG(WARNING) << error.description;
  }
  return sdi;
}

void pingCallback(WRTCSession* clientSession, NetworkManager* nm,
                  std::shared_ptr<std::string> messageBuffer) {
  if (!messageBuffer || !messageBuffer.get()) {
    LOG(WARNING) << "WsServer: Invalid messageBuffer";
    return;
  }

  if (!clientSession) {
    LOG(WARNING) << "WSServer invalid clientSession!";
    return;
  }

  // const std::string incomingStr = beast::buffers_to_string(messageBuffer->data());
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "pingCallback incomingMsg=" << messageBuffer.get()->c_str();

  // send same message back (ping-pong)
  clientSession->send(messageBuffer);
}

void keepaliveCallback(WRTCSession* clientSession, NetworkManager* nm,
                       std::shared_ptr<std::string> messageBuffer) {
  if (!messageBuffer || !messageBuffer.get()) {
    LOG(WARNING) << "WsServer: Invalid messageBuffer";
    return;
  }

  if (!clientSession) {
    LOG(WARNING) << "WSServer invalid clientSession!";
    return;
  }

  // const std::string incomingStr = beast::buffers_to_string(messageBuffer->data());
  /*LOG(INFO) << std::this_thread::get_id() << ":"
            << "keepaliveCallback from " << clientSession->getId()
            << " incomingMsg=" << messageBuffer.get()->c_str();*/
}

void serverTimeCallback(WRTCSession* clientSession, NetworkManager* nm,
                        std::shared_ptr<std::string> messageBuffer) {
  if (!messageBuffer || !messageBuffer.get()) {
    LOG(WARNING) << "WsServer: Invalid messageBuffer";
    return;
  }

  if (!clientSession) {
    LOG(WARNING) << "WSServer invalid clientSession!";
    return;
  }

  // const std::string incomingStr = beast::buffers_to_string(messageBuffer->data());
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "pingCallback incomingMsg=" << messageBuffer.get()->c_str();

  std::chrono::system_clock::time_point nowTp = std::chrono::system_clock::now();
  std::time_t t = std::chrono::system_clock::to_time_t(nowTp);

  std::string msg = "serverTimeCallback: ";
  msg += std::ctime(&t);

  // send same message back (ping-pong)
  clientSession->send(msg);
}

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

WRTCServer::WRTCServer(NetworkManager* nm, const utils::config::ServerConfig& serverConfig)
    : nm_(nm), webrtcConf_(webrtc::PeerConnectionInterface::RTCConfiguration()),
      webrtcGamedataOpts_(webrtc::PeerConnectionInterface::RTCOfferAnswerOptions()),
      dataChannelCount_(0) {
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
    ice_servers[0].uri = "stun:stun.l.google.com:19302";
    ice_servers[1].uri = "stun:stun1.l.google.com:19302";
    ice_servers[2].uri = "stun:stun2.l.google.com:19305";
    ice_servers[3].uri = "stun:stun01.sipphone.com";
    ice_servers[4].uri = "stun:stunserver.org";
    // TODO ice_server.username = "xxx";
    // TODO ice_server.password = kTurnPassword;
    // TODO
    // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    resetWebRtcConfig(
        {ice_servers[0], ice_servers[1], ice_servers[2], ice_servers[3], ice_servers[4]});
  }
}

WRTCServer::~WRTCServer() { // TODO: virtual
  // auto call Quit()?
}

void WRTCServer::InitAndRun() {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::InitAndRun";
  if (!rtc::InitRandom(rtc::Time32())) {
    LOG(WARNING) << "Error in InitializeSSL()";
  }

  // Create the PeerConnectionFactory.
  if (!rtc::InitializeSSL()) {
    LOG(WARNING) << "Error in InitializeSSL()";
  }

  // See
  // https://github.com/pristineio/webrtc-mirror/blob/7a5bcdffaab90a05bc1146b2b1ea71c004e54d71/webrtc/rtc_base/thread.cc
  networkThread_ = rtc::Thread::CreateWithSocketServer(); // reset(new rtc::Thread());
  networkThread_->SetName("network_thread1", nullptr);

  workerThread_ = rtc::Thread::Create();
  workerThread_->SetName("worker_thread1", nullptr);

  signalingThread_ = rtc::Thread::Create();
  signalingThread_->SetName("signaling_thread1", nullptr);

  wrtcNetworkManager_.reset(new rtc::BasicNetworkManager());
  socketFactory_.reset(
      new rtc::BasicPacketSocketFactory(networkThread_.get())); // or _workerThread?

  RTC_CHECK(networkThread_->Start()) << "Failed to start network_thread";
  LOG(INFO) << "Started network_thread";
  RTC_CHECK(workerThread_->Start()) << "Failed to start worker_thread";
  LOG(INFO) << "Started worker_thread";
  RTC_CHECK(signalingThread_->Start()) << "Failed to start signaling_thread";
  LOG(INFO) << "Started signaling_thread";

  // see https://github.com/sourcey/libsourcey/blob/master/src/webrtc/src/peerfactorycontext.cpp#L53
  peerConnectionFactory_ = webrtc::CreateModularPeerConnectionFactory(
      networkThread_.get(), workerThread_.get(), signalingThread_.get(), nullptr, nullptr, nullptr);

  // TODO: pass other settings e.t.c.
  // see https://github.com/sourcey/libsourcey/blob/master/src/webrtc/src/peerfactorycontext.cpp#L53
  /*peerConnectionFactory_ = webrtc::CreateModularPeerConnectionFactory(nullptr, nullptr, nullptr,
                                                                      nullptr, nullptr, nullptr);*/

  LOG(INFO) << "Created PeerConnectionFactory";

  if (peerConnectionFactory_.get() == nullptr) {
    LOG(WARNING) << "Error: Could not create CreatePeerConnectionFactory.";
    return;
  }
  /*rtc::Thread* signalingThread = rtc::Thread::Current();
  signalingThread->Run();*/

  /* // TODO
   * while(handleWrtcMsgs) {

  }*/

  LOG(WARNING) << "WebRTC thread finished";
}

void WRTCServer::resetWebRtcConfig(
    const std::vector<webrtc::PeerConnectionInterface::IceServer>& iceServers) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::resetWebRtcConfig";

  // settings for game-server messaging
  webrtcGamedataOpts_.offer_to_receive_audio = false;
  webrtcGamedataOpts_.offer_to_receive_video = false;

  // SCTP supports unordered data.
  // Unordered data is unimportant for multiplayer games.
  dataChannelConf_.ordered = false;
  // maxRetransmits is 0, because if a message didn’t arrive, we don’t care.
  dataChannelConf_.maxRetransmits = 0;
  // data_channel_config.maxRetransmitTime = options.maxRetransmitTime; // TODO
  // TODO
  // True if the channel has been
  // externally negotiated
  // data_channel_config.negotiated = true;
  // data_channel_config.id = 0;

  // set servers
  webrtcConf_.servers.clear();
  for (const auto& ice_server : iceServers) {
    // ICE is the protocol chosen for NAT traversal in WebRTC.
    // SEE
    // https://chromium.googlesource.com/external/webrtc/+/lkgr/pc/peerconnection.cc
    LOG(INFO) << "added ice_server " << ice_server.uri;
    webrtcConf_.servers.push_back(ice_server);
    // If set to true, use RTP data channels instead of SCTP.
    // TODO(deadbeef): Remove this. We no longer commit to supporting RTP data
    // channels, though some applications are still working on moving off of
    // them.
    // RTP data channel rate limited!
    // https://richard.to/programming/sending-images-with-webrtc-data-channels.html
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

void WRTCServer::finishThreads() {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::Quit";
  // CloseDataChannel?
  // https://webrtc.googlesource.com/src/+/master/examples/unityplugin/simple_peer_connection.cc
  if (networkThread_.get())
    networkThread_->Quit();
  if (signalingThread_.get())
    signalingThread_->Quit();
  if (workerThread_.get())
    workerThread_->Quit();
  // webrtc_thread.reset(); // TODO
  rtc::CleanupSSL();
}

/*rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> WRTCServer::getPCF() const {
  return peerConnectionFactory_;
}*/

webrtc::PeerConnectionInterface::RTCConfiguration WRTCServer::getWRTCConf() const {
  return webrtcConf_;
}

void WRTCServer::addDataChannelCount(uint32_t count) {
  LOG(INFO) << "WRTCServer::onDataChannelOpen";
  dataChannelCount_ += count;
  // TODO: check overflow
  LOG(INFO) << "WRTCServer::onDataChannelOpen: data channel count: " << dataChannelCount_;
}

void WRTCServer::subDataChannelCount(uint32_t count) {
  LOG(INFO) << "WRTCServer::onDataChannelOpen";
  dataChannelCount_ -= count;
  // TODO: check overflow
  LOG(INFO) << "WRTCServer::onDataChannelOpen: data channel count: " << dataChannelCount_;
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
    for (auto& sessionkv : getSessions()) {
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
    auto sessionsCopy = getSessions();
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

// TODO: freezes with big queue
// TODO:  webrtc::JsepTransportController::OnMessage
void WRTCServer::handleIncomingMessages() {
  LOG(INFO) << "WRTCServer::handleIncomingMessages getSessionsCount " << getSessionsCount();
  doToAllSessions([&](const std::string& sessId, std::shared_ptr<WRTCSession> session) {
    if (!session || !session.get()) {
      LOG(WARNING) << "WRTCServer::handleAllPlayerMessages: trying to "
                      "use non-existing session";
      // NOTE: unregisterSession must be automatic!
      unregisterSession(sessId);
      return;
    }
    /*if (!session->isOpen() && session->fullyCreated()) {
      LOG(WARNING) << "WsServer::handleAllPlayerMessages: !session->isOpen()";
      // NOTE: unregisterSession must be automatic!
      unregisterSession(session->getId());
      return;
    }*/
    // TODO: check timer expiry independantly from handleIncomingMessages
    const bool isTimerExpired =
        boost::posix_time::second_clock::local_time() > session->timerDeadline;
    if (isTimerExpired) {
      LOG(WARNING) << "WsServer::handleAllPlayerMessages: session timer expired";
      unregisterSession(session->getId());
      return;
    }
    auto msgs = session->getReceivedMessages();
    if (!msgs || !msgs.get()) {
      LOG(WARNING) << "WsServer::handleAllPlayerMessages: invalid session->getReceivedMessages()";
      return;
    }
    // LOG(INFO) << "doToAllSessions for " << session->getId();
    msgs->DispatchQueued();
  });
}

/**
 * @brief removes session from list of valid sessions
 *
 * @param id id of session to be removed
 */
void WRTCServer::unregisterSession(const std::string& id) {
  const std::string idCopy = id; // unknown lifetime, use idCopy
  std::shared_ptr<WRTCSession> sess = getSessById(idCopy);
  {
    if (!removeSessById(idCopy)) {
      // throw std::runtime_error(
      LOG(WARNING) << "WRTCServer::unregisterSession: trying to unregister non-existing session";
      // NOTE: continue cleanup with saved shared_ptr
    }
    if (!sess || !sess.get()) {
      // throw std::runtime_error(
      LOG(WARNING) << "WRTCServer::unregisterSession: session already deleted";
      return;
    }
  }
  LOG(WARNING) << "WrtcServer: unregistered " << idCopy;
}

void WRTCServer::runThreads(const utils::config::ServerConfig& serverConfig) {
  webrtcThread_ = std::thread(&WRTCServer::webRtcSignalThreadEntry, this);
}

// The thread entry point for the WebRTC thread. This sets the WebRTC thread as
// the signaling thread and creates a worker thread in the background.
void WRTCServer::webRtcSignalThreadEntry() { InitAndRun(); }

void WRTCServer::setRemoteDescriptionAndCreateAnswer(WsSession* clientWsSession, NetworkManager* nm,
                                                     const rapidjson::Document& message_object) {
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

  const std::string& webrtcConnId = nextWrtcSessionId();
  const std::string wsConnId = clientWsSession->getId(); // copy... //// <<<<

  // std::unique_ptr<cricket::PortAllocator> port_allocator(new
  // cricket::BasicPortAllocator(new rtc::BasicNetworkManager()));
  // port_allocator->SetPortRange(60000, 60001);
  // TODO ice_server.username = "xxx";
  // TODO ice_server.password = kTurnPassword;
  LOG(INFO) << "creating peer_connection...";
  {

    LOG(INFO) << "creating peerConnectionObserver...";
    /**
     * @brief WebRTC peer connection observer
     * Used to create WebRTC session paired with WebSocket session
     */
    std::shared_ptr<PCO> peerConnectionObserver_ =
        std::make_shared<PCO>(nm, webrtcConnId, wsConnId); // TODO: to private

    if (!peerConnectionObserver_ || !peerConnectionObserver_.get()) {
      LOG(WARNING) << "empty peer_connection_observer";
      return;
    }

    createdWRTCSession = std::make_shared<WRTCSession>(nm, webrtcConnId, wsConnId);

    // see https://github.com/sourcey/libsourcey/blob/master/src/webrtc/include/scy/webrtc/peer.h
    // see https://github.com/sourcey/libsourcey/blob/master/src/webrtc/src/peer.cpp
    std::unique_ptr<cricket::BasicPortAllocator> portAllocator_;

    if (!nm->getWRTC()->wrtcNetworkManager_.get() || !nm->getWRTC()->socketFactory_.get()) {
      LOG(WARNING) << "WRTCServer::setRemoteDescriptionAndCreateAnswer: invalid "
                      "wrtcNetworkManager_ or socketFactory_";
      return;
    }

    if (!portAllocator_) {
      portAllocator_.reset(new cricket::BasicPortAllocator(nm->getWRTC()->wrtcNetworkManager_.get(),
                                                           nm->getWRTC()->socketFactory_.get()));
    }
    portAllocator_->SetPortRange(/* minPort */ 60000, /* maxPort */ 60001);

    if (!portAllocator_ || !portAllocator_.get()) {
      LOG(WARNING) << "WRTCServer::setRemoteDescriptionAndCreateAnswer: invalid portAllocator_";
      return;
    }

    {
      rtc::CritScope lock(&nm->getWRTC()->pcMutex_);
      // prevents pci_ garbage collection by 'operator='
      createdWRTCSession->pci_ = nm->getWRTC()->peerConnectionFactory_->CreatePeerConnection(
          nm->getWRTC()->getWRTCConf(), std::move(portAllocator_), nullptr,
          peerConnectionObserver_.get());
      if (!createdWRTCSession->pci_ || !createdWRTCSession->pci_.get()) {
        LOG(WARNING) << "WRTCServer::setRemoteDescriptionAndCreateAnswer: empty PCI";
        return;
      }
    }

    // save at WRTCSession to prevent garbage collection
    createdWRTCSession->peerConnectionObserver_ = peerConnectionObserver_;

    LOG(INFO) << "creating WRTCSession...";
    {
      auto isSessCreated = nm->getWRTC()->addSession(webrtcConnId, createdWRTCSession);
      if (!isSessCreated) {
        LOG(WARNING) << "setRemoteDescriptionAndCreateAnswer: Can`t create session ";
        return;
      }
      clientWsSession->pairToWRTCSession(createdWRTCSession);
      createdWRTCSession->setObservers();
      LOG(INFO) << "updating peerConnections_ for webrtcConnId = " << webrtcConnId;
    }
  }

  createdWRTCSession->createDCI();

  createdWRTCSession->updateDataChannelState();

  LOG(INFO) << "createSessionDescriptionFromJson...";
  webrtc::SessionDescriptionInterface* clientSessionDescription =
      createSessionDescriptionFromJson(message_object);
  if (!clientSessionDescription) {
    LOG(WARNING) << "empty clientSessionDescription!";
    return;
  }

  createdWRTCSession->SetRemoteDescription(clientSessionDescription);

  createdWRTCSession->CreateAnswer();
}

} // namespace net
} // namespace utils

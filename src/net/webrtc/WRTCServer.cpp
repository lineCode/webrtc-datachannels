#include "net/webrtc/WRTCServer.hpp" // IWYU pragma: associated
#include "algorithm/DispatchQueue.hpp"
#include "algorithm/NetworkOperation.hpp"
#include "algorithm/StringUtils.hpp"
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

WRTCServer::WRTCServer(NetworkManager* nm)
    : nm_(nm), webrtcConf_(webrtc::PeerConnectionInterface::RTCConfiguration()),
      webrtcGamedataOpts_(webrtc::PeerConnectionInterface::RTCOfferAnswerOptions()),
      dataChannelCount_(0), dataChannelstate_(webrtc::DataChannelInterface::kClosed) {
  WRTCQueue_ =
      std::make_shared<algo::DispatchQueue>(std::string{"WebRTC Server Dispatch Queue"}, 0);
  // peerConnectionObserver_ = std::make_unique<PCO>(nm);
}

WRTCServer::~WRTCServer() { // TODO: virtual
  // auto call Quit()?
}

void WRTCServer::InitAndRun() {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::InitAndRun";
  // Create the PeerConnectionFactory.
  if (!rtc::InitializeSSL()) {
    LOG(WARNING) << "Error in InitializeSSL()";
  }
  // TODO: free memory
  // TODO: reset(new rtc::Thread()) ???
  // SEE
  // https://github.com/pristineio/webrtc-mirror/blob/7a5bcdffaab90a05bc1146b2b1ea71c004e54d71/webrtc/rtc_base/thread.cc

  // network_thread = rtc::Thread::CreateWithSocketServer();
  // network_thread.reset(rtc::Thread::Current());//reset(new rtc::Thread());
  networkThread_ = rtc::Thread::CreateWithSocketServer(); // reset(new rtc::Thread());
  // network_thread =
  // std::make_unique<rtc::Thread>(rtc::Thread::CreateWithSocketServer()); //
  // rtc::Thread::CreateWithSocketServer();
  networkThread_->SetName("network_thread 1", nullptr);

  // worker_thread.reset(rtc::Thread::Current());//reset(new rtc::Thread());
  /*worker_thread = rtc::Thread::Create();
  worker_thread->SetName("worker_thread 1", nullptr);*/

  // signaling_thread.reset(rtc::Thread::Current());
  signalingThread_ = rtc::Thread::Create();
  signalingThread_->SetName("signaling_thread 1", nullptr);

  RTC_CHECK(networkThread_->Start()) << "Failed to start network_thread";
  LOG(INFO) << "Started network_thread";
  /*RTC_CHECK(worker_thread->Start()) << "Failed to start worker_thread";
  LOG(INFO) << "Started worker_thread";*/
  RTC_CHECK(signalingThread_->Start()) << "Failed to start signaling_thread";
  LOG(INFO) << "Started signaling_thread";

  std::unique_ptr<webrtc::CallFactoryInterface> call_factory(webrtc::CreateCallFactory());
  std::unique_ptr<webrtc::RtcEventLogFactoryInterface> rtc_event_log_factory(
      webrtc::CreateRtcEventLogFactory());
  /*worker_thread->Invoke<bool>(RTC_FROM_HERE, [this]() {
    this->peer_connection_factory = webrtc::CreateModularPeerConnectionFactory(
    this->network_thread.get(), //rtc::Thread* network_thread,
    this->worker_thread.get(), //rtc::Thread* worker_thread,
    this->signaling_thread.get(), //rtc::Thread::Current(), //nullptr,
  //std::move(signaling_thread), //rtc::Thread* signaling_thread, nullptr,
  //std::unique_ptr<cricket::MediaEngineInterface> media_engine, nullptr,
  //std::unique_ptr<CallFactoryInterface> call_factory, nullptr
  //std::unique_ptr<RtcEventLogFactoryInterface> event_log_factory);
  );

    return true;
  });*/

  /*WebRtcVideoEncoderFactory* video_encoder_factory = nullptr;
  WebRtcVideoDecoderFactory* video_decoder_factory = nullptr;
  rtc::NetworkMonitorFactory* network_monitor_factory = nullptr;
  //auto audio_encoder_factory = webrtc::CreateAudioEncoderFactory();
  //auto audio_decoder_factory = webrtc::CreateAudioDecoderFactory();

  webrtc::AudioDeviceModule* adm = nullptr;
  rtc::scoped_refptr<webrtc::AudioMixer> audio_mixer = nullptr;*/
  /*std::unique_ptr<cricket::MediaEngineInterface>
     media_engine(cricket::WebRtcMediaEngineFactory::Create( adm,
     audio_encoder_factory, audio_decoder_factory, video_encoder_factory,
      video_decoder_factory, audio_mixer,
     webrtc::AudioProcessingBuilder().Create()));//webrtc::AudioProcessing::Create()));*/
  // SEE
  // https://cs.chromium.org/chromium/src/third_party/webrtc/pc/peerconnectioninterface_unittest.cc?q=MediaEngineInterface&dr=C&l=645
  /*auto media_engine = std::unique_ptr<cricket::MediaEngineInterface>(
        cricket::WebRtcMediaEngineFactory::Create(
            adm, audio_encoder_factory,
            audio_decoder_factory, std::move(video_encoder_factory),
            std::move(video_decoder_factory), nullptr,
            webrtc::AudioProcessingBuilder().Create()));*/
  /*std::unique_ptr<cricket::MediaEngineInterface>
     media_engine(cricket::WebRtcMediaEngineFactory::Create( nullptr,
     webrtc::CreateBuiltinAudioEncoderFactory(),
      webrtc::CreateBuiltinAudioDecoderFactory(),
      webrtc::CreateBuiltinVideoEncoderFactory(),
      webrtc::CreateBuiltinVideoDecoderFactory(), nullptr,
      webrtc::AudioProcessingBuilder().Create()));*/

  /*
   * RTCPeerConnection that serves as the starting point
   * to create any type of connection, data channel or otherwise.
   **/
  /*peer_connection_factory = webrtc::CreateModularPeerConnectionFactory(
    network_thread.get(), //rtc::Thread* network_thread,
    network_thread.get(), ////worker_thread.get(), //rtc::Thread* worker_thread,
    signaling_thread.get(), //rtc::Thread::Current(), //nullptr,
  //std::move(signaling_thread), //rtc::Thread* signaling_thread, nullptr,
  //std::move(media_engine), //std::unique_ptr<cricket::MediaEngineInterface>
  media_engine, std::move(call_factory), //std::unique_ptr<CallFactoryInterface>
  call_factory, std::move(rtc_event_log_factory)
  //std::unique_ptr<RtcEventLogFactoryInterface> event_log_factory);
  );*/
  peerConnectionFactory_ = webrtc::CreateModularPeerConnectionFactory(nullptr, nullptr, nullptr,
                                                                      nullptr, nullptr, nullptr);
  LOG(INFO) << "Created PeerConnectionFactory";
  if (peerConnectionFactory_.get() == nullptr) {
    LOG(WARNING) << "Error: Could not create CreatePeerConnectionFactory.";
    // return?
  }
  rtc::Thread* signalingThread = rtc::Thread::Current();
  signalingThread->Run();

  /*webrtc::PeerConnectionFactoryInterface::Options webRtcOptions;
  // NOTE: you cannot disable encryption for SCTP-based data channels. And
  RTP-based data channels are not in the spec.
  // SEE https://groups.google.com/forum/#!topic/discuss-webrtc/_6TCUy775PM
  webRtcOptions.disable_encryption = true;
  peer_connection_factory->SetOptions(webRtcOptions);*/
  // signaling_thread->set_socketserver(&socket_server);
  // signaling_thread.reset(new rtc::Thread(&socket_server));
  /*signaling_thread->Run();
  LOG(INFO) << "Run worker_thread";
  network_thread->Run();
  LOG(INFO) << "Run network_thread";
  worker_thread->Run();
  LOG(INFO) << "Run worker_thread";*/
  /*if (!signaling_thread->Start()) {
    // TODO
  }
  if (!network_thread->Start()) {
    // TODO
  }
  if (!worker_thread->Start()) {
    // TODO
  }*/
  // signaling_thread->set_socketserver(nullptr);

  /*LOG(INFO) << "started WebRTC event loop";
  bool shouldQuitWRTCThread = false;
  while(!shouldQuitWRTCThread){
    WRTCQueue->dispatch_thread_handler();
  }

  LOG(INFO) << "WebRTC thread finished";*/
}

void WRTCServer::resetWebRtcConfig(
    const std::vector<webrtc::PeerConnectionInterface::IceServer>& iceServers) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::resetWebRtcConfig";
  // settings for game-server messaging
  webrtcGamedataOpts_.offer_to_receive_audio = false;
  webrtcGamedataOpts_.offer_to_receive_video = false;

  // SCTP supports unordered data. Unordered data is unimportant for multiplayer
  // games.
  dataChannelConf_.ordered = false;
  // maxRetransmits is 0, because if a message didn’t arrive, we don’t care.
  dataChannelConf_.maxRetransmits = 0;
  // data_channel_config.maxRetransmitTime = options.maxRetransmitTime; // TODO
  // TODO
  // data_channel_config.negotiated = true; // True if the channel has been
  // externally negotiated data_channel_config.id = 0;

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

void WRTCServer::Quit() {
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

rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> WRTCServer::getPCF() const {
  return peerConnectionFactory_;
}

webrtc::PeerConnectionInterface::RTCConfiguration WRTCServer::getWRTCConf() const {
  return webrtcConf_;
}

std::shared_ptr<WRTCSession> WRTCServer::getSessById(const std::string& webrtcConnId) {
  auto it = peerConnections_.find(webrtcConnId);
  if (it != peerConnections_.end()) {
    return it->second;
  }
  LOG(WARNING) << "WRTCServer::getSessByIdt: unknown session with id = " << webrtcConnId;
  return nullptr;
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

} // namespace net
} // namespace utils

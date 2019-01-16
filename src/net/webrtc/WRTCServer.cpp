#include "net/webrtc/WRTCServer.hpp" // IWYU pragma: associated
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include "net/webrtc/Observers.hpp"
#include "net/websockets/WsSession.hpp"
#include "net/websockets/WsSessionManager.hpp"
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <webrtc/api/mediaconstraintsinterface.h>
#include <webrtc/api/peerconnectioninterface.h>
#include <webrtc/api/rtpreceiverinterface.h>
#include <webrtc/api/rtpsenderinterface.h>
#include <webrtc/api/videosourceproxy.h>
#include <webrtc/media/base/mediaengine.h>
#include <webrtc/media/base/videocapturer.h>
#include <webrtc/pc/webrtcsdp.h>
#include <webrtc/rtc_base/bind.h>
#include <webrtc/rtc_base/checks.h>
#include <webrtc/rtc_base/event_tracer.h>
#include <webrtc/rtc_base/logging.h>
#include <webrtc/rtc_base/logsinks.h>
#include <webrtc/rtc_base/messagequeue.h>
#include <webrtc/rtc_base/networkmonitor.h>
#include <webrtc/rtc_base/rtccertificategenerator.h>
#include <webrtc/rtc_base/ssladapter.h>
#include <webrtc/rtc_base/stringutils.h>
#include <webrtc/system_wrappers/include/field_trial.h>

namespace utils {
namespace net {

// TODO: global?
// Constants for Session Description Protocol (SDP)
static const char kCandidateSdpMidName[] = "sdpMid";
static const char kCandidateSdpMlineIndexName[] = "sdpMLineIndex";
static const char kCandidateSdpName[] = "candidate";

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// TODO
void CloseDataChannel(
    rtc::scoped_refptr<webrtc::DataChannelInterface>& in_data_channel) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "CloseDataChannel";
  if (in_data_channel.get()) {
    in_data_channel->UnregisterObserver();
    in_data_channel->Close();
  }
  in_data_channel = nullptr;
}

webrtc::SessionDescriptionInterface*
createSessionDescriptionFromJson(const rapidjson::Document& message_object) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "createSessionDescriptionFromJson";
  webrtc::SdpParseError error;
  std::string sdp = message_object["payload"]["sdp"].GetString();
  LOG(INFO) << "sdp =" << sdp;
  // TODO: free memory?
  // TODO: handle error?
  webrtc::SessionDescriptionInterface* sdi =
      webrtc::CreateSessionDescription("offer", sdp, &error);
  if (sdi == nullptr) {
    LOG(INFO) << "createSessionDescriptionFromJson:: SDI IS NULL"
              << error.description.c_str();
  }
  LOG(INFO) << error.description;
  return sdi;
}

webrtc::IceCandidateInterface*
createIceCandidateFromJson(const rapidjson::Document& message_object) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "createIceCandidateFromJson";
  std::string candidate =
      message_object["payload"][kCandidateSdpName].GetString();
  LOG(INFO) << "got candidate from client =" << candidate;
  int sdp_mline_index =
      message_object["payload"][kCandidateSdpMlineIndexName].GetInt();
  std::string sdp_mid =
      message_object["payload"][kCandidateSdpMidName].GetString();
  webrtc::SdpParseError error;
  // TODO: free memory?
  // TODO: handle error?
  webrtc::IceCandidateInterface* iceCanidate =
      webrtc::CreateIceCandidate(sdp_mid, sdp_mline_index, candidate, &error);
  if (iceCanidate == nullptr) {
    LOG(INFO) << "createIceCandidateFromJson:: iceCanidate IS NULL"
              << error.description.c_str();
  }
  LOG(INFO) << error.description;
  return iceCanidate;
}

WRTCServer::WRTCServer(NetworkManager* nm)
    : nm_(nm), dataChannelstate(webrtc::DataChannelInterface::kClosed),
      webrtcConfiguration(webrtc::PeerConnectionInterface::RTCConfiguration()),
      data_channel_count(0),
      webrtc_gamedata_options(
          webrtc::PeerConnectionInterface::RTCOfferAnswerOptions()) {
  WRTCQueue = new DispatchQueue(std::string{"WebRTC Server Dispatch Queue"}, 0);
  data_channel_observer = new utils::net::DCO(*this);
  create_session_description_observer = new CSDO(*this);
  peer_connection_observer = new PCO(*this);
  local_description_observer = new SSDO();
  remote_description_observer = new SSDO();
  // thread_checker_.DetachFromThread();
}

bool WRTCServer::sendDataViaDataChannel(const std::string& data) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::sendDataViaDataChannel";
  webrtc::DataBuffer buffer(data);
  sendDataViaDataChannel(buffer);
  return true;
}

bool WRTCServer::sendDataViaDataChannel(const webrtc::DataBuffer& buffer) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::sendDataViaDataChannel";
  if (!data_channel.get()) {
    LOG(INFO) << "Data channel is not established";
    return false;
  }
  if (!data_channel->Send(buffer)) {
    switch (data_channel->state()) {
    case webrtc::DataChannelInterface::kConnecting: {
      LOG(INFO) << "Unable to send arraybuffer. DataChannel is connecting";
      break;
    }
    case webrtc::DataChannelInterface::kOpen: {
      LOG(INFO) << "Unable to send arraybuffer.";
      break;
    }
    case webrtc::DataChannelInterface::kClosing: {
      LOG(INFO) << "Unable to send arraybuffer. DataChannel is closing";
      break;
    }
    case webrtc::DataChannelInterface::kClosed: {
      LOG(INFO) << "Unable to send arraybuffer. DataChannel is closed";
      break;
    }
    default: {
      LOG(INFO) << "Unable to send arraybuffer. DataChannel unknown state";
      break;
    }
    }
  }
  return true;
}

webrtc::DataChannelInterface::DataState WRTCServer::updateDataChannelState() {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::updateDataChannelState";
  dataChannelstate = data_channel->state();
  return dataChannelstate;
}

void WRTCServer::setLocalDescription(webrtc::SessionDescriptionInterface* sdi) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::setLocalDescription";
  // store the server’s own answer
  {
    rtc::CritScope lock(&pc_mutex_);
    if (!local_description_observer) {
      LOG(INFO) << "empty local_description_observer";
    }
    peer_connection->SetLocalDescription(local_description_observer, sdi);
  }
  // setLocalDescription(&local_description_observer, sdi);
}

void WRTCServer::createAndAddIceCandidate(
    const rapidjson::Document& message_object) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::createAndAddIceCandidate";
  // rtc::CritScope lock(&pc_mutex_);
  auto candidate_object = createIceCandidateFromJson(message_object);
  // sends IceCandidate to Client via websockets, see OnIceCandidate
  {
    rtc::CritScope lock(&pc_mutex_);
    peer_connection->AddIceCandidate(candidate_object);
  }
}

void WRTCServer::SetRemoteDescriptionAndCreateAnswer(
    const rapidjson::Document& message_object) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::SetRemoteDescriptionAndCreateAnswer";

  // std::unique_ptr<cricket::PortAllocator> port_allocator(new
  // cricket::BasicPortAllocator(new rtc::BasicNetworkManager()));
  // port_allocator->SetPortRange(60000, 60001);
  // TODO ice_server.username = "xxx";
  // TODO ice_server.password = kTurnPassword;
  LOG(INFO) << "creating peer_connection...";

  /*// TODO: kEnableDtlsSrtp? READ
  https://webrtchacks.com/webrtc-must-implement-dtls-srtp-but-must-not-implement-sdes/
  webrtc::FakeConstraints constraints;
  constraints.AddOptional(
      webrtc::MediaConstraintsInterface::kEnableDtlsSrtp, "true");*/
  // webrtc_thread->
  // SEE
  // https://github.com/BrandonMakin/Godot-Module-WebRTC/blob/3dd6e66555c81c985f81a1eea54bbc039461c6bf/godot/modules/webrtc/webrtc_peer.cpp

  {
    if (!peer_connection_observer) {
      LOG(INFO) << "empty peer_connection_observer";
    }
    rtc::CritScope lock(&pc_mutex_);
    peer_connection = peer_connection_factory->CreatePeerConnection(
        webrtcConfiguration, nullptr, nullptr, peer_connection_observer);
  }
  // TODO: make global
  // std::unique_ptr<cricket::PortAllocator> allocator(new
  // cricket::BasicPortAllocator(new rtc::BasicNetworkManager(), new
  // rtc::CustomSocketFactory()));
  /*peer_connection = new
     webrtc::PeerConnection(peer_connection_factory->CreatePeerConnection(configuration,
     std::move(port_allocator), nullptr,
      &m_WRTC->observer->peer_connection_observer));*/
  /*peer_connection =
    peer_connection_factory->CreatePeerConnection(configuration,
    std::move(port_allocator), nullptr,
    &m_WRTC->observer->peer_connection_observer);*/
  LOG(INFO) << "created CreatePeerConnection.";
  /*if (!peer_connection.get() == nullptr) {
    LOG(INFO) << "Error: Could not create CreatePeerConnection.";
    // return?
  }*/
  LOG(INFO) << "created peer_connection";

  LOG(INFO) << "creating DataChannel...";
  const std::string data_channel_lable = "dc";
  data_channel = peer_connection->CreateDataChannel(data_channel_lable,
                                                    &data_channel_config);
  LOG(INFO) << "created DataChannel";
  LOG(INFO) << "registering observer...";
  if (!data_channel_observer) {
    LOG(INFO) << "empty data_channel_observer";
  }
  data_channel->RegisterObserver(data_channel_observer);
  LOG(INFO) << "registered observer";

  //
  LOG(INFO) << "createSessionDescriptionFromJson...";
  auto client_session_description =
      createSessionDescriptionFromJson(message_object);
  if (!client_session_description) {
    LOG(INFO) << "empty client_session_description!";
  }
  LOG(INFO) << "SetRemoteDescription...";

  {
    rtc::CritScope lock(&pc_mutex_);
    LOG(INFO) << "pc_mutex_...";
    if (!peer_connection) {
      LOG(INFO) << "empty peer_connection!";
    }
    /*if (!remote_description_observer) {
      LOG(INFO) << "empty remote_description_observer!";
    }*/
    if (!remote_description_observer) {
      LOG(INFO) << "empty remote_description_observer";
    }
    peer_connection->SetRemoteDescription(remote_description_observer,
                                          client_session_description);
  }
  // peer_connection->CreateAnswer(&m_WRTC->observer->create_session_description_observer,
  // nullptr);

  // SEE
  // https://chromium.googlesource.com/external/webrtc/+/HEAD/pc/peerconnection_signaling_unittest.cc
  // SEE
  // https://books.google.ru/books?id=jfNfAwAAQBAJ&pg=PT208&lpg=PT208&dq=%22RTCOfferAnswerOptions%22+%22CreateAnswer%22&source=bl&ots=U1c5gQMSlU&sig=UHb_PlSNaDpfim6dlx0__a8BZ8Y&hl=ru&sa=X&ved=2ahUKEwi2rsbqq7PfAhU5AxAIHWZCA204ChDoATAGegQIBxAB#v=onepage&q=%22RTCOfferAnswerOptions%22%20%22CreateAnswer%22&f=false
  // READ https://www.w3.org/TR/webrtc/#dom-rtcofferansweroptions
  // SEE https://gist.github.com/MatrixMuto/e37f50567e4b9b982dd8673a1e49dcbe

  // Answer to client offer with server media capabilities
  // Answer sents by websocker in OnAnswerCreated
  // CreateAnswer will trigger the OnSuccess event of
  // CreateSessionDescriptionObserver. This will in turn invoke our
  // OnAnswerCreated callback for sending the answer to the client.
  LOG(INFO) << "peer_connection->CreateAnswer...";
  {
    rtc::CritScope lock(&pc_mutex_);
    if (!create_session_description_observer) {
      LOG(INFO) << "empty create_session_description_observer";
    }
    peer_connection->CreateAnswer(create_session_description_observer,
                                  webrtc_gamedata_options);
  }
  LOG(INFO) << "peer_connection created answer";
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
  network_thread =
      rtc::Thread::CreateWithSocketServer(); // reset(new rtc::Thread());
  // network_thread =
  // std::make_unique<rtc::Thread>(rtc::Thread::CreateWithSocketServer()); //
  // rtc::Thread::CreateWithSocketServer();
  network_thread->SetName("network_thread 1", nullptr);

  // worker_thread.reset(rtc::Thread::Current());//reset(new rtc::Thread());
  /*worker_thread = rtc::Thread::Create();
  worker_thread->SetName("worker_thread 1", nullptr);*/

  // signaling_thread.reset(rtc::Thread::Current());
  signaling_thread = rtc::Thread::Create();
  signaling_thread->SetName("signaling_thread 1", nullptr);

  RTC_CHECK(network_thread->Start()) << "Failed to start network_thread";
  LOG(INFO) << "Started network_thread";
  /*RTC_CHECK(worker_thread->Start()) << "Failed to start worker_thread";
  LOG(INFO) << "Started worker_thread";*/
  RTC_CHECK(signaling_thread->Start()) << "Failed to start signaling_thread";
  LOG(INFO) << "Started signaling_thread";

  std::unique_ptr<webrtc::CallFactoryInterface> call_factory(
      webrtc::CreateCallFactory());
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
  peer_connection_factory = webrtc::CreateModularPeerConnectionFactory(
      nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
  LOG(INFO) << "Created PeerConnectionFactory";
  if (peer_connection_factory.get() == nullptr) {
    LOG(INFO) << "Error: Could not create CreatePeerConnectionFactory.";
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
  webrtc_gamedata_options.offer_to_receive_audio = false;
  webrtc_gamedata_options.offer_to_receive_video = false;

  // SCTP supports unordered data. Unordered data is unimportant for multiplayer
  // games.
  data_channel_config.ordered = false;
  // maxRetransmits is 0, because if a message didn’t arrive, we don’t care.
  data_channel_config.maxRetransmits = 0;
  // data_channel_config.maxRetransmitTime = options.maxRetransmitTime; // TODO
  // TODO
  // data_channel_config.negotiated = true; // True if the channel has been
  // externally negotiated data_channel_config.id = 0;

  // set servers
  webrtcConfiguration.servers.clear();
  for (const auto& ice_server : iceServers) {
    // ICE is the protocol chosen for NAT traversal in WebRTC.
    // SEE
    // https://chromium.googlesource.com/external/webrtc/+/lkgr/pc/peerconnection.cc
    LOG(INFO) << "added ice_server " << ice_server.uri;
    webrtcConfiguration.servers.push_back(ice_server);
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

bool WRTCServer::isDataChannelOpen() {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::isDataChannelOpen";
  return dataChannelstate == webrtc::DataChannelInterface::kOpen;
}

void WRTCServer::Quit() {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::Quit";
  // CloseDataChannel?
  // https://webrtc.googlesource.com/src/+/master/examples/unityplugin/simple_peer_connection.cc
  if (network_thread.get())
    network_thread->Quit();
  if (signaling_thread.get())
    signaling_thread->Quit();
  if (worker_thread.get())
    worker_thread->Quit();
  // webrtc_thread.reset(); // TODO
  rtc::CleanupSSL();
}

/*
 * TODO: Use Facebook’s lock-free queue && process the network messages in batch
 * server also contains a glaring inefficiency in its
 * immediate handling of data channel messages in the OnDataChannelMessage
 *callback. The cost of doing so in our example is negligible, but in an actual
 *game server, the message handler will be a more costly function that must
 *interact with state. The message handling function will then block the
 *signaling thread from processing any other messages on the wire during its
 *execution. To avoid this, I recommend pushing all messages onto a thread-safe
 *message queue, and during the next game tick, the main game loop running in a
 *different thread can process the network messages in batch. I use Facebook’s
 *lock-free queue in my own game server for this.
 **/
// Callback for when the server receives a message on the data channel.
void WRTCServer::OnDataChannelMessage(const webrtc::DataBuffer& buffer) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::OnDataChannelMessage";
  std::string data(buffer.data.data<char>(), buffer.data.size());
  LOG(INFO) << data;
  webrtc::DataChannelInterface::DataState state = data_channel->state();
  if (state != webrtc::DataChannelInterface::kOpen) {
    LOG(INFO) << "OnDataChannelMessage: data channel not open!";
    // TODO return false;
  }
  // std::string str = "pong";
  // webrtc::DataBuffer resp(rtc::CopyOnWriteBuffer(str.c_str(), str.length()),
  // false /* binary */);

  // send back?
  sendDataViaDataChannel(buffer);
}

// Callback for when the data channel is successfully created. We need to
// re-register the updated data channel here.
void WRTCServer::OnDataChannelCreated(
    rtc::scoped_refptr<webrtc::DataChannelInterface> channel) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::OnDataChannelCreated";
  data_channel = channel;
  if (!data_channel_observer) {
    LOG(INFO) << "empty data_channel_observer";
  }
  data_channel->RegisterObserver(data_channel_observer);
  // data_channel_count++; // NEED HERE?
}

// TODO: on closed

// Callback for when the STUN server responds with the ICE candidates.
// Sends by websocket JSON containing { candidate, sdpMid, sdpMLineIndex }
void WRTCServer::OnIceCandidate(
    const webrtc::IceCandidateInterface* candidate) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::OnIceCandidate";
  std::string candidate_str;
  if (!candidate->ToString(&candidate_str)) {
    LOG(INFO) << "Failed to serialize candidate";
    // LOG(LS_ERROR) << "Failed to serialize candidate";
    // return;
  }
  candidate->ToString(&candidate_str);
  rapidjson::Document message_object;
  message_object.SetObject();
  message_object.AddMember(
      "type",
      rapidjson::StringRef(CANDIDATE_OPERATION.operationCodeStr_.c_str()),
      message_object.GetAllocator());
  rapidjson::Value candidate_value;
  candidate_value.SetString(rapidjson::StringRef(candidate_str.c_str()));
  rapidjson::Value sdp_mid_value;
  sdp_mid_value.SetString(rapidjson::StringRef(candidate->sdp_mid().c_str()));
  rapidjson::Value message_payload;
  message_payload.SetObject();
  // candidate: Host candidate for RTP on UDP - in this ICE line our browser is
  // giving its host candidates- the IP example "candidate":
  // "candidate:1791031112 1 udp 2122260223 10.10.15.25 57339 typ host
  // generation 0 ufrag say/ network-id 1 network-cost 10",
  message_payload.AddMember(kCandidateSdpName, candidate_value,
                            message_object.GetAllocator());
  // sdpMid: audio or video or ...
  message_payload.AddMember(kCandidateSdpMidName, sdp_mid_value,
                            message_object.GetAllocator());
  message_payload.AddMember(kCandidateSdpMlineIndexName,
                            candidate->sdp_mline_index(),
                            message_object.GetAllocator());
  message_object.AddMember("payload", message_payload,
                           message_object.GetAllocator());
  rapidjson::StringBuffer strbuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  message_object.Accept(writer);
  std::string payload = strbuf.GetString();
  // webSocketServer->send(payload);

  // TODDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO
  // need to send to 1 player ONLY!
  nm_->getWsSessionManager()->sendToAll(payload);
  // <<<<<<<
  //
  // TODO: webrtc message queue!
}

void WRTCServer::onDataChannelOpen() {
  LOG(INFO) << "WRTCServer::onDataChannelOpen";
  data_channel_count++;
  LOG(INFO) << "WRTCServer::onDataChannelOpen: data channel count: "
            << data_channel_count;
}

void WRTCServer::onDataChannelClose() {
  LOG(INFO) << "WRTCServer::onDataChannelClose";
  data_channel_count--;
  LOG(INFO) << "WRTCServer::onDataChannelClose: data channel count: "
            << data_channel_count;
}

// Callback for when the answer is created. This sends the answer back to the
// client.
void WRTCServer::OnAnswerCreated(webrtc::SessionDescriptionInterface* sdi) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::OnAnswerCreated";
  if (sdi == nullptr) {
    LOG(INFO) << "WRTCServer::OnAnswerCreated INVALID SDI";
  }
  LOG(INFO) << "OnAnswerCreated";
  // store the server’s own answer
  setLocalDescription(sdi);
  // Apologies for the poor code ergonomics here; I think rapidjson is just
  // verbose.
  std::string offer_string;
  sdi->ToString(&offer_string);
  rapidjson::Document message_object;
  message_object.SetObject();
  message_object.AddMember(
      "type", rapidjson::StringRef(ANSWER_OPERATION.operationCodeStr_.c_str()),
      message_object.GetAllocator());
  rapidjson::Value sdp_value;
  sdp_value.SetString(rapidjson::StringRef(offer_string.c_str()));

  rapidjson::Value message_payload;
  message_payload.SetObject();
  message_payload.AddMember(
      "type",
      "answer", // rapidjson::StringRef(ANSWER_OPERATION.operationCodeStr_.c_str()),
      message_object.GetAllocator());
  message_payload.AddMember("sdp", sdp_value, message_object.GetAllocator());
  message_object.AddMember("payload", message_payload,
                           message_object.GetAllocator());

  rapidjson::StringBuffer strbuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  message_object.Accept(writer);
  std::string payload = strbuf.GetString();
  // webSocketServer->send(payload);

  // TODDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO
  // send to 1 player!
  nm_->getWsSessionManager()->sendToAll(payload);
}

} // namespace net
} // namespace utils

// This a minimal fully functional example for setting up a server written in C++ that communicates
// with clients via WebRTC data channels. This uses WebSockets to perform the WebRTC handshake
// (offer/accept SDP) with the client. We only use WebSockets for the initial handshake because TCP
// often presents too much latency in the context of real-time action games. WebRTC data channels,
// on the other hand, allow for unreliable and unordered message sending via SCTP.
//
// Author: brian@brkho.com

#include "observers.h"

//#define BOOST_NO_EXCEPTIONS
//#define BOOST_NO_RTTI

/*#include <boost/asio/signal_set.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/foreach.hpp>
//#include <boost/lockfree/spsc_queue.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>*/
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <webrtc/api/peerconnectioninterface.h>
#include <webrtc/rtc_base/physicalsocketserver.h>
#include <webrtc/rtc_base/ssladapter.h>
#include <webrtc/rtc_base/thread.h>
#include <webrtc/media/engine/webrtcmediaengine.h>
#include "webrtc/rtc_base/third_party/sigslot/sigslot.h"
#include "webrtc/rtc_base/strings/json.h"
#include "webrtc/p2p/base/basicpacketsocketfactory.h"
#include "webrtc/p2p/client/basicportallocator.h" // https://github.com/sourcey/libsourcey/blob/master/src/webrtc/src/peerfactorycontext.cpp
#include "webrtc/pc/peerconnectionfactory.h"
#include "webrtc/pc/peerconnection.h"
//#include "webrtc/rtc_base/gunit.h"
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <iostream>
#include <thread>

// WebSocket++ types are gnarly.
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

/*namespace po = boost::program_options;
namespace pt = boost::property_tree;
namespace ws = boost::beast::websocket;*/

// Some forward declarations.
void OnDataChannelCreated(rtc::scoped_refptr<webrtc::DataChannelInterface> channel);
void OnIceCandidate(const webrtc::IceCandidateInterface* candidate);
void OnDataChannelMessage(const webrtc::DataBuffer& buffer);
void OnAnswerCreated(webrtc::SessionDescriptionInterface* desc);

typedef websocketpp::server<websocketpp::config::asio> WebSocketServer;
typedef WebSocketServer::message_ptr message_ptr;

// TODO: refactor global variables

// The WebSocket server being used to handshake with the clients.
WebSocketServer ws_server;
// The peer conncetion factory that sets up signaling and worker threads. It is also used to create
// the PeerConnection.
rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory;
// The socket that the signaling thread and worker thread communicate on.
//CustomSocketServer socket_server;
//rtc::PhysicalSocketServer socket_server;
// thread for WebRTC listening loop.
std::thread webrtc_thread;
// thread for WebSocket listening loop.
std::thread websockets_thread;
// The WebSocket connection handler that uniquely identifies one of the connections that the
// WebSocket has open. If you want to have multiple connections, you will need to store more than
// one of these.
websocketpp::connection_hdl websocket_connection_handler;
// The peer connection through which we engage in the Session Description Protocol (SDP) handshake.
rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection; // TODO: multiple clients?
// The data channel used to communicate.
rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel;
// 
WebRtcObserver webRtcObserver;
// The observer that responds to data channel events.
// webrtc::DataChannelObserver for data channel events like receiving SCTP messages.
DCO data_channel_observer(OnDataChannelMessage);
// The observer that responds to session description creation events.
// webrtc::CreateSessionDescriptionObserver for creating an offer or answer.
CSDO create_session_description_observer(OnAnswerCreated);
// The observer that responds to session description set events. We don't really use this one here.
// webrtc::SetSessionDescriptionObserver for acknowledging and storing an offer or answer.
SSDO local_description_observer;
SSDO remove_description_observer;
// The observer that responds to peer connection events.
// webrtc::PeerConnectionObserver for peer connection events such as receiving ICE candidates.
PCO peer_connection_observer(OnDataChannelCreated, OnIceCandidate);
//rtc::scoped_refptr<PCO> peer_connection_observer = new rtc::RefCountedObject<PCO>(OnDataChannelCreated, OnIceCandidate);

webrtc::PeerConnectionInterface::RTCConfiguration webrtcConfiguration;

webrtc::PeerConnectionInterface::RTCOfferAnswerOptions webrtc_gamedata_options;

// TODO: global config var
webrtc::DataChannelInit data_channel_config;

int data_channel_count = 0;

// Constants for Session Description Protocol (SDP)
const char kCandidateSdpMidName[] = "sdpMid";
const char kCandidateSdpMlineIndexName[] = "sdpMLineIndex";
const char kCandidateSdpName[] = "candidate";

// TODO: free memory
//rtc::Thread* signaling_thread;
/*
 * The signaling thread handles the bulk of WebRTC computation;
 * it creates all of the basic components and fires events we can consume by calling the observer methods
*/
static std::unique_ptr<rtc::Thread> signaling_thread;
static std::unique_ptr<rtc::Thread> network_thread;
/*
 * worker thread, on the other hand, is delegated resource-intensive tasks
 * such as media streaming to ensure that the signaling thread doesn’t get blocked
*/
static std::unique_ptr<rtc::Thread> worker_thread;
//rtc::Thread* network_thread_;

// Callback for when the data channel is successfully created. We need to re-register the updated
// data channel here.
void OnDataChannelCreated(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) {
  std::cout << "OnDataChannelCreated" << std::endl;
  data_channel = channel;
  data_channel->RegisterObserver(&data_channel_observer);
  data_channel_count++;
}

// Callback for when the STUN server responds with the ICE candidates.
// Sends by websocket JSON containing { candidate, sdpMid, sdpMLineIndex }
void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
  std::cout << "OnIceCandidate" << std::endl;
  std::string candidate_str;
  if (!candidate->ToString(&candidate_str)) {
      std::cout << "Failed to serialize candidate" << std::endl;
      //LOG(LS_ERROR) << "Failed to serialize candidate";
      //return;
  }
  candidate->ToString(&candidate_str);
  rapidjson::Document message_object;
  message_object.SetObject();
  message_object.AddMember("type", "candidate", message_object.GetAllocator());
  rapidjson::Value candidate_value;
  candidate_value.SetString(rapidjson::StringRef(candidate_str.c_str()));
  rapidjson::Value sdp_mid_value;
  sdp_mid_value.SetString(rapidjson::StringRef(candidate->sdp_mid().c_str()));
  rapidjson::Value message_payload;
  message_payload.SetObject();
  // candidate: Host candidate for RTP on UDP - in this ICE line our browser is giving its host candidates- the IP
  // example "candidate": "candidate:1791031112 1 udp 2122260223 10.10.15.25 57339 typ host generation 0 ufrag say/ network-id 1 network-cost 10",
  message_payload.AddMember(kCandidateSdpName, candidate_value, message_object.GetAllocator());
  // sdpMid: audio or video or ...
  message_payload.AddMember(kCandidateSdpMidName, sdp_mid_value, message_object.GetAllocator());
  message_payload.AddMember(kCandidateSdpMlineIndexName, candidate->sdp_mline_index(),
      message_object.GetAllocator());
  message_object.AddMember("payload", message_payload, message_object.GetAllocator());
  rapidjson::StringBuffer strbuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  message_object.Accept(writer);
  std::string payload = strbuf.GetString();
  ws_server.send(websocket_connection_handler, payload, websocketpp::frame::opcode::value::text);
}

/*
 * TODO: Use Facebook’s lock-free queue && process the network messages in batch
 * server also contains a glaring inefficiency in its
 * immediate handling of data channel messages in the OnDataChannelMessage callback. 
 * The cost of doing so in our example is negligible, but in an actual game server,
 * the message handler will be a more costly function that must interact with state.
 * The message handling function will then block the signaling thread
 * from processing any other messages on the wire during its execution.
 * To avoid this, I recommend pushing all messages onto a thread-safe message queue,
 * and during the next game tick, the main game loop running in a different thread can process the network messages in batch.
 * I use Facebook’s lock-free queue in my own game server for this.
**/
// Callback for when the server receives a message on the data channel.
void OnDataChannelMessage(const webrtc::DataBuffer& buffer) {
  std::cout << "OnDataChannelMessage" << std::endl;
  std::string data(buffer.data.data<char>(), buffer.data.size());
  std::cout << data << std::endl;
  webrtc::DataChannelInterface::DataState state = data_channel->state();
  if (state != webrtc::DataChannelInterface::kOpen) {
    std::cout << "OnDataChannelMessage: data channel not open!" << std::endl;
    // TODO return false;  
  }
  // std::string str = "pong";
  // webrtc::DataBuffer resp(rtc::CopyOnWriteBuffer(str.c_str(), str.length()), false /* binary */);

  // send back?  
  data_channel->Send(buffer);
}

bool sendDataViaDataChannel(const std::string& data) {
  if (!data_channel.get()) {
    std::cout << "Data channel is not established" << std::endl;
    return false;
  }
  webrtc::DataBuffer buffer(data);
  data_channel->Send(buffer);
  return true;
}

void CloseDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface>& in_data_channel) {
  if (in_data_channel.get()) {
    in_data_channel->UnregisterObserver();
    in_data_channel->Close();
  }
  in_data_channel = nullptr;
}

// Callback for when the answer is created. This sends the answer back to the client.
void OnAnswerCreated(webrtc::SessionDescriptionInterface* desc) {
  std::cout << "OnAnswerCreated" << std::endl;
  // store the server’s own answer
  peer_connection->SetLocalDescription(&local_description_observer, desc);
  // Apologies for the poor code ergonomics here; I think rapidjson is just verbose.
  std::string offer_string;
  desc->ToString(&offer_string);
  rapidjson::Document message_object;
  message_object.SetObject();
  message_object.AddMember("type", "answer", message_object.GetAllocator());
  rapidjson::Value sdp_value;
  sdp_value.SetString(rapidjson::StringRef(offer_string.c_str()));
  rapidjson::Value message_payload;
  message_payload.SetObject();
  message_payload.AddMember("type", "answer", message_object.GetAllocator());
  message_payload.AddMember("sdp", sdp_value, message_object.GetAllocator());
  message_object.AddMember("payload", message_payload, message_object.GetAllocator());
  rapidjson::StringBuffer strbuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  message_object.Accept(writer);
  std::string payload = strbuf.GetString();
  ws_server.send(websocket_connection_handler, payload, websocketpp::frame::opcode::value::text);
}

// Used to test websockets performance
void handleWebsocketsPing(websocketpp::connection_hdl hdl, message_ptr msg) {
  std::cout << "type == offer" << std::endl;
  std::string id = msg->get_payload().c_str();
  std::cout << "id =" << id << std::endl;
  ws_server.send(websocket_connection_handler, id, websocketpp::frame::opcode::value::text);
}

webrtc::SessionDescriptionInterface* createSessionDescriptionFromJson(rapidjson::Document& message_object){
  webrtc::SdpParseError error;
  std::string sdp = message_object["payload"]["sdp"].GetString();
  std::cout << "sdp =" << sdp << std::endl;
  // TODO: free memory?
  // TODO: handle error?
  return webrtc::CreateSessionDescription("offer", sdp, &error);
}

webrtc::IceCandidateInterface* createIceCandidateFromJson(rapidjson::Document& message_object){
  std::string candidate = message_object["payload"][kCandidateSdpName].GetString();
  std::cout << "got candidate from client =" << candidate << std::endl;
  int sdp_mline_index = message_object["payload"][kCandidateSdpMlineIndexName].GetInt();
  std::string sdp_mid = message_object["payload"][kCandidateSdpMidName].GetString();
  webrtc::SdpParseError error;
  // TODO: free memory?
  // TODO: handle error?
  return webrtc::CreateIceCandidate(sdp_mid, sdp_mline_index, candidate, &error);
}

/*
 * Callback for when the WebSocket server receives a message from the client.
 * The peer connection needs 2 things from the server to set up the WebRTC connection:
 * 1 a session description
 * 2 ice candidates.
 * We get this info through webrtc messages. 
**/
void OnWebSocketMessage(WebSocketServer* /* s */, websocketpp::connection_hdl hdl, message_ptr msg) {
  std::cout << "OnWebSocketMessage" << std::endl;
  websocket_connection_handler = hdl;
  rapidjson::Document message_object;
  message_object.Parse(msg->get_payload().c_str());
  std::cout << msg->get_payload().c_str() << std::endl;
  // Probably should do some error checking on the JSON object.
  std::string type = message_object["type"].GetString();
  if (type == "ping") {
    handleWebsocketsPing(hdl, msg);
  } else if (type == "offer") {
    // TODO: don`t create datachennel for same client twice?
    std::cout << "type == offer" << std::endl;

    //std::unique_ptr<cricket::PortAllocator> port_allocator(new cricket::BasicPortAllocator(new rtc::BasicNetworkManager()));
    //port_allocator->SetPortRange(60000, 60001);
    // TODO ice_server.username = "xxx";
    // TODO ice_server.password = kTurnPassword;
    std::cout << "creating peer_connection..." << std::endl;

    /*// TODO: kEnableDtlsSrtp? READ https://webrtchacks.com/webrtc-must-implement-dtls-srtp-but-must-not-implement-sdes/
    webrtc::FakeConstraints constraints;
    constraints.AddOptional(
        webrtc::MediaConstraintsInterface::kEnableDtlsSrtp, "true");*/

    // SEE https://github.com/BrandonMakin/Godot-Module-WebRTC/blob/3dd6e66555c81c985f81a1eea54bbc039461c6bf/godot/modules/webrtc/webrtc_peer.cpp
    peer_connection = peer_connection_factory->CreatePeerConnection(webrtcConfiguration, nullptr, nullptr,
        &peer_connection_observer);
    // TODO: make global
    //std::unique_ptr<cricket::PortAllocator> allocator(new cricket::BasicPortAllocator(new rtc::BasicNetworkManager(), new rtc::CustomSocketFactory()));
    /*peer_connection = new webrtc::PeerConnection(peer_connection_factory->CreatePeerConnection(configuration, std::move(port_allocator), nullptr,
        &peer_connection_observer));*/
    /*peer_connection = peer_connection_factory->CreatePeerConnection(configuration, std::move(port_allocator), nullptr,
      &peer_connection_observer);*/
    std::cout << "created CreatePeerConnection." << std::endl;
    /*if (!peer_connection.get() == nullptr) {
      std::cout << "Error: Could not create CreatePeerConnection." << std::endl;
      // return?
    }*/
    std::cout << "created peer_connection" << std::endl;

    std::cout << "creating DataChannel..." << std::endl;
    const std::string data_channel_lable = "dc";
    data_channel = peer_connection->CreateDataChannel(data_channel_lable, &data_channel_config);
    std::cout << "created DataChannel" << std::endl;
    std::cout << "registering observer..." << std::endl;
    data_channel->RegisterObserver(&data_channel_observer);
    std::cout << "registered observer" << std::endl;

    auto session_description = createSessionDescriptionFromJson(message_object);
    peer_connection->SetRemoteDescription(&remove_description_observer, session_description);
    //peer_connection->CreateAnswer(&create_session_description_observer, nullptr);

    // SEE https://chromium.googlesource.com/external/webrtc/+/HEAD/pc/peerconnection_signaling_unittest.cc
    // SEE https://books.google.ru/books?id=jfNfAwAAQBAJ&pg=PT208&lpg=PT208&dq=%22RTCOfferAnswerOptions%22+%22CreateAnswer%22&source=bl&ots=U1c5gQMSlU&sig=UHb_PlSNaDpfim6dlx0__a8BZ8Y&hl=ru&sa=X&ved=2ahUKEwi2rsbqq7PfAhU5AxAIHWZCA204ChDoATAGegQIBxAB#v=onepage&q=%22RTCOfferAnswerOptions%22%20%22CreateAnswer%22&f=false
    // READ https://www.w3.org/TR/webrtc/#dom-rtcofferansweroptions
    // SEE https://gist.github.com/MatrixMuto/e37f50567e4b9b982dd8673a1e49dcbe

    // Answer to client offer with server media capabilities
    // Answer sents by websocker in OnAnswerCreated
    // CreateAnswer will trigger the OnSuccess event of CreateSessionDescriptionObserver.
    // This will in turn invoke our OnAnswerCreated callback for sending the answer to the client.
    peer_connection->CreateAnswer(&create_session_description_observer, webrtc_gamedata_options);
  } else if (type == "candidate") {
    // Server receives Client’s ICE candidates, then finds its own ICE candidates & sends them to Client
    std::cout << "type == candidate" << std::endl;
    auto candidate_object = createIceCandidateFromJson(message_object);
    // sends IceCandidate to Client via websockets, see OnIceCandidate
    peer_connection->AddIceCandidate(candidate_object);
  } else {
    std::cout << "Unrecognized WebSocket message type." << std::endl;
  }
}

// The thread entry point for the WebSockets thread.
void WebSocketThreadEntry() {
  // In a real game server, you would run the WebSocket server as a separate thread so your main
  // process can handle the game loop.
  ws_server.set_message_handler(bind(OnWebSocketMessage, &ws_server, ::_1, ::_2));
  ws_server.init_asio();
  ws_server.clear_access_channels(websocketpp::log::alevel::all);
  ws_server.set_reuse_addr(true);
  ws_server.listen(8080);
  ws_server.start_accept();
  std::cout << "Websocket server stated on 8080 port" << std::endl;
  // I don't do it here, but you should gracefully handle closing the connection.
  ws_server.run();
}

void resetWebRtcConfig(const std::vector<std::string>& uris = std::vector<std::string>{"stun:stun.l.google.com:19302"}) {
  // settings for game-server messaging
  webrtc_gamedata_options.offer_to_receive_audio = false;
  webrtc_gamedata_options.offer_to_receive_video = false;


  // SCTP supports unordered data. Unordered data is unimportant for multiplayer games.
  data_channel_config.ordered = false;
  // maxRetransmits is 0, because if a message didn’t arrive, we don’t care.
  data_channel_config.maxRetransmits = 0;
  // data_channel_config.maxRetransmitTime = options.maxRetransmitTime; // TODO

  // set servers
  webrtcConfiguration.servers.clear();
  for (const auto& it : uris) {
    // ICE is the protocol chosen for NAT traversal in WebRTC. 
    webrtc::PeerConnectionInterface::IceServer ice_server;
    // TODO >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    ice_server.uri = it;
    // TODO ice_server.username = "xxx";
    // TODO ice_server.password = kTurnPassword;
    // TODO <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    webrtcConfiguration.servers.push_back(ice_server);
    // If set to true, use RTP data channels instead of SCTP.
    // TODO(deadbeef): Remove this. We no longer commit to supporting RTP data
    // channels, though some applications are still working on moving off of
    // them.
    // RTP data channel rate limited! https://richard.to/programming/sending-images-with-webrtc-data-channels.html
    // webrtcConfiguration.enable_rtp_data_channel = true;
    // Can be used to disable DTLS-SRTP. This should never be done, but can be
    // useful for testing purposes, for example in setting up a loopback call
    // with a single PeerConnection.
    // webrtcConfiguration.enable_dtls_srtp = false;
  }
}

// The thread entry point for the WebRTC thread. This sets the WebRTC thread as the signaling thread
// and creates a worker thread in the background.
void WebRtcSignalThreadEntry() {
  resetWebRtcConfig();

  // Create the PeerConnectionFactory.
  rtc::InitializeSSL();
  // TODO: free memory
  // TODO: reset(new rtc::Thread()) ???
  signaling_thread.reset(rtc::Thread::Current());
  signaling_thread->SetName("signaling_thread 1", nullptr);
  network_thread.reset(rtc::Thread::Current());//reset(new rtc::Thread());
  network_thread->SetName("network_thread 1", nullptr);
  worker_thread.reset(rtc::Thread::Current());//reset(new rtc::Thread());
  worker_thread->SetName("worker_thread 1", nullptr);
  /*
  * RTCPeerConnection that serves as the starting point
  * to create any type of connection, data channel or otherwise.
  **/
  peer_connection_factory = webrtc::CreateModularPeerConnectionFactory(
    network_thread.get(), //rtc::Thread* network_thread,
    worker_thread.get(), //rtc::Thread* worker_thread,
    signaling_thread.get(), //rtc::Thread::Current(), //nullptr, //std::move(signaling_thread), //rtc::Thread* signaling_thread,
    nullptr, //std::unique_ptr<cricket::MediaEngineInterface> media_engine,
    nullptr, //std::unique_ptr<CallFactoryInterface> call_factory,
    nullptr //std::unique_ptr<RtcEventLogFactoryInterface> event_log_factory);
  );
  std::cout << "Created PeerConnectionFactory" << std::endl;
  if (peer_connection_factory.get() == nullptr) {
    std::cout << "Error: Could not create CreatePeerConnectionFactory." << std::endl;
    // return?
  }
  //signaling_thread->set_socketserver(&socket_server);
  //signaling_thread.reset(new rtc::Thread(&socket_server));
  signaling_thread->Run();
  network_thread->Run();
  worker_thread->Run();
  //signaling_thread->set_socketserver(nullptr);
  rtc::CleanupSSL();
}
#ifdef NOPE
void create_factory_then_run() {
  std::cout << "create_factory_then_run" << std::endl;
  rtc::InitializeSSL();
/*
    LOG(INFO) << "CreatePeerConnectionFactory";
    factory = webrtc::CreatePeerConnectionFactory();

    PeerConnectionInterface::RTCConfiguration config;
    PeerConnectionInterface::IceServer server;
    server.uri = "stun:stun.l.google.com:19302";
    config.servers.push_back(server);

//    config.enable_dtls_srtp = rtc::Optional<bool>(false);

    LOG(INFO) << "CreatePeerConnection";
    peer_conn = factory->CreatePeerConnection(config, NULL, NULL, observer);

    rtc::Thread *thread = rtc::Thread::Current();
    rtc::PhysicalSocketServer socketServer;
    thread->Run();
*/
  //webrtc_thread = std::thread(WebRtcSignalThreadEntry);
  signaling_thread = new rtc::Thread(&socket_server);
  //network_thread = new rtc::Thread();
  std::cout << "Started signaling thread" << std::endl;
  //network_thread->Start();
  signaling_thread->SetName("signaling_thread 1");
  signaling_thread->Start();
  std::cout << "Created signaling thread" << std::endl;
  //webrtc_thread.reset(signaling_thread);
  // SEE https://github.com/jitsi/webrtc/blob/master/pc/peerconnectionfactory_unittest.cc#L95
#ifdef NOPE
  peer_connection_factory = webrtc::CreatePeerConnectionFactory(
          nullptr /* network_thread */, nullptr /* worker_thread */,
          nullptr /* signaling_thread */, nullptr /* default_adm */,
          webrtc::CreateBuiltinAudioEncoderFactory(),
          webrtc::CreateBuiltinAudioDecoderFactory(),
          webrtc::CreateBuiltinVideoEncoderFactory(),
          webrtc::CreateBuiltinVideoDecoderFactory(), nullptr /* audio_mixer */,
          nullptr /* audio_processing */);
#endif
  /*std::unique_ptr<CallFactoryInterface> call_factory = CreateCallFactory();
  std::unique_ptr<RtcEventLogFactoryInterface> event_log_factory =
      CreateRtcEventLogFactory();*/
  /*peer_connection_factory = webrtc::CreateModularPeerConnectionFactory(
    nullptr, //rtc::Thread::Current(), //nullptr,//network_thread_, // rtc::Thread* network_thread,
    nullptr, //rtc::Thread::Current(), //nullptr,//rtc::Thread::Current(), // rtc::Thread* worker_thread,
    nullptr, //signaling_thread,
    nullptr, //std::move(media_engine), // std::unique_ptr<cricket::MediaEngineInterface> media_engine,
    nullptr, //std::move(call_factory), // std::unique_ptr<CallFactoryInterface> call_factory,
    nullptr //std::move(event_log_factory) // std::unique_ptr<RtcEventLogFactoryInterface> event_log_factory
  );*/
  peer_connection_factory = webrtc::CreateModularPeerConnectionFactory(
    nullptr, //rtc::Thread::Current(), //nullptr,//network_thread_, // rtc::Thread* network_thread,
    nullptr, //rtc::Thread::Current(), //rtc::Thread::Current(), //nullptr,//rtc::Thread::Current(), // rtc::Thread* worker_thread,
    nullptr, //signaling_thread,
    nullptr, //std::move(media_engine), // std::unique_ptr<cricket::MediaEngineInterface> media_engine,
    nullptr, //std::move(call_factory), // std::unique_ptr<CallFactoryInterface> call_factory,
    nullptr //std::move(event_log_factory) // std::unique_ptr<RtcEventLogFactoryInterface> event_log_factory
  );
  // rtc::Thread*, rtc::Thread*, rtc::Thread*, rtc::scoped_refptr<webrtc::AudioDeviceModule>, rtc::scoped_refptr<webrtc::AudioEncoderFactory>, rtc::scoped_refptr<webrtc::AudioDecoderFactory>, std::unique_ptr<webrtc::VideoEncoderFactory>, std::unique_ptr<webrtc::VideoDecoderFactory>, rtc::scoped_refptr<webrtc::AudioMixer>, rtc::scoped_refptr<webrtc::AudioProcessing>

  /*peer_connection_factory = webrtc::CreatePeerConnectionFactory(
    network_thread_, // rtc::Thread* worker_and_network_thread,
    rtc::Thread::Current(), // rtc::Thread* signaling_thread,
    nullptr, // AudioDeviceModule* default_adm,
    webrtc::CreateBuiltinAudioEncoderFactory(), // rtc::scoped_refptr<AudioEncoderFactory> audio_encoder_factory,
    webrtc::CreateBuiltinAudioDecoderFactory(), // rtc::scoped_refptr<AudioDecoderFactory> audio_decoder_factory,
    nullptr, // cricket::WebRtcVideoEncoderFactory* video_encoder_factory,
    nullptr // cricket::WebRtcVideoDecoderFactory* video_decoder_factory)
  );*/
  std::cout << "Created PeerConnectionFactory" << std::endl;
  if (peer_connection_factory.get() == nullptr) {
    std::cout << "Error: Could not create CreatePeerConnectionFactory." << std::endl;
    // return?
  }
  rtc::Thread::Current()->Run();
  rtc::CleanupSSL();
}
#endif
// Main entry point of the code.
int main() {
  std::cout << std::this_thread::get_id() << ":"
            << "Main thread" << std::endl;
  std::cout << "Initialized SSL" << std::endl;

  //webrtc_thread = std::thread(create_factory_then_run);


  /*
   * Nothing is stopping a server from being one of the WebRTC peers.
   * WebSockets makes things easier, because the server is always reachable,
   * which means STUN and TURN are not really necessary.
  **/
  // TODO: mutexes? Message Queue?
  {
    // Runs WebRTC listening loop
    webrtc_thread = std::thread(WebRtcSignalThreadEntry);
    // Runs WebSocket listening loop
    websockets_thread = std::thread(WebSocketThreadEntry);
  }

  while(true){
   // TODO
  }

  // websockets
  //websockets_thread.reset();
  // webrtc
  // CloseDataChannel? https://webrtc.googlesource.com/src/+/master/examples/unityplugin/simple_peer_connection.cc
  network_thread->Quit();
  signaling_thread->Quit();
  worker_thread->Quit();
  //webrtc_thread.reset();
}

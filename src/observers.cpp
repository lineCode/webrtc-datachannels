
#include "observers.h"

// TODO
void CloseDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface>& in_data_channel) {
  if (in_data_channel.get()) {
    in_data_channel->UnregisterObserver();
    in_data_channel->Close();
  }
  in_data_channel = nullptr;
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

void WSServer::Quit(){
  // websockets
  //websockets_thread.reset();
}

void WSServer::InitAndRun() {
  ws_server.set_message_handler(bind(WSServer::OnWebSocketMessage, m_WRTC, this, &ws_server, ::_1, ::_2));
  ws_server.init_asio();
  ws_server.clear_access_channels(websocketpp::log::alevel::all);
  ws_server.set_reuse_addr(true);
  ws_server.listen(8080);
  ws_server.start_accept();
  std::cout << "Websocket server stated on 8080 port" << std::endl;
  // I don't do it here, but you should gracefully handle closing the connection.
  ws_server.run();
}

// Used to test websockets performance
void WSServer::handleWebsocketsPing(websocketpp::connection_hdl hdl, message_ptr msg) {
  std::cout << "type == offer" << std::endl;
  std::string id = msg->get_payload().c_str();
  std::cout << "id =" << id << std::endl;
  this->send(id);
}

void WSServer::send(const std::string payload) {
  ws_server.send(websocket_connection_handler, payload, websocketpp::frame::opcode::value::text);
}

/*
 * Callback for when the WebSocket server receives a message from the client.
 * The peer connection needs 2 things from the server to set up the WebRTC connection:
 * 1 a session description
 * 2 ice candidates.
 * We get this info through webrtc messages. 
**/
void WSServer::OnWebSocketMessage(WRTCServer* m_WRTC, WSServer* m_WS, WebSocketServer* /* s */, websocketpp::connection_hdl hdl, message_ptr msg) {
  std::cout << "OnWebSocketMessage" << std::endl;
  m_WS->websocket_connection_handler = hdl;
  rapidjson::Document message_object;
  message_object.Parse(msg->get_payload().c_str());
  std::cout << msg->get_payload().c_str() << std::endl;
  // Probably should do some error checking on the JSON object.
  std::string type = message_object["type"].GetString();
  if (type == "ping") {
    m_WS->handleWebsocketsPing(hdl, msg);
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
    m_WRTC->peer_connection = m_WRTC->peer_connection_factory->CreatePeerConnection(m_WRTC->webrtcConfiguration, nullptr, nullptr,
        &m_WRTC->observer->peer_connection_observer);
    // TODO: make global
    //std::unique_ptr<cricket::PortAllocator> allocator(new cricket::BasicPortAllocator(new rtc::BasicNetworkManager(), new rtc::CustomSocketFactory()));
    /*peer_connection = new webrtc::PeerConnection(peer_connection_factory->CreatePeerConnection(configuration, std::move(port_allocator), nullptr,
        &m_WRTC->observer->peer_connection_observer));*/
    /*peer_connection = peer_connection_factory->CreatePeerConnection(configuration, std::move(port_allocator), nullptr,
      &m_WRTC->observer->peer_connection_observer);*/
    std::cout << "created CreatePeerConnection." << std::endl;
    /*if (!peer_connection.get() == nullptr) {
      std::cout << "Error: Could not create CreatePeerConnection." << std::endl;
      // return?
    }*/
    std::cout << "created peer_connection" << std::endl;

    std::cout << "creating DataChannel..." << std::endl;
    const std::string data_channel_lable = "dc";
    m_WRTC->data_channel = m_WRTC->peer_connection->CreateDataChannel(data_channel_lable, &m_WRTC->data_channel_config);
    std::cout << "created DataChannel" << std::endl;
    std::cout << "registering observer..." << std::endl;
    m_WRTC->data_channel->RegisterObserver(&m_WRTC->observer->data_channel_observer);
    std::cout << "registered observer" << std::endl;

    auto session_description = createSessionDescriptionFromJson(message_object);
    m_WRTC->peer_connection->SetRemoteDescription(&m_WRTC->observer->remote_description_observer, session_description);
    //peer_connection->CreateAnswer(&m_WRTC->observer->create_session_description_observer, nullptr);

    // SEE https://chromium.googlesource.com/external/webrtc/+/HEAD/pc/peerconnection_signaling_unittest.cc
    // SEE https://books.google.ru/books?id=jfNfAwAAQBAJ&pg=PT208&lpg=PT208&dq=%22RTCOfferAnswerOptions%22+%22CreateAnswer%22&source=bl&ots=U1c5gQMSlU&sig=UHb_PlSNaDpfim6dlx0__a8BZ8Y&hl=ru&sa=X&ved=2ahUKEwi2rsbqq7PfAhU5AxAIHWZCA204ChDoATAGegQIBxAB#v=onepage&q=%22RTCOfferAnswerOptions%22%20%22CreateAnswer%22&f=false
    // READ https://www.w3.org/TR/webrtc/#dom-rtcofferansweroptions
    // SEE https://gist.github.com/MatrixMuto/e37f50567e4b9b982dd8673a1e49dcbe

    // Answer to client offer with server media capabilities
    // Answer sents by websocker in OnAnswerCreated
    // CreateAnswer will trigger the OnSuccess event of CreateSessionDescriptionObserver.
    // This will in turn invoke our OnAnswerCreated callback for sending the answer to the client.
    m_WRTC->peer_connection->CreateAnswer(&m_WRTC->observer->create_session_description_observer, m_WRTC->webrtc_gamedata_options);
  } else if (type == "candidate") {
    // Server receives Client’s ICE candidates, then finds its own ICE candidates & sends them to Client
    std::cout << "type == candidate" << std::endl;
    auto candidate_object = createIceCandidateFromJson(message_object);
    // sends IceCandidate to Client via websockets, see OnIceCandidate
    m_WRTC->peer_connection->AddIceCandidate(candidate_object);
  } else {
    std::cout << "Unrecognized WebSocket message type." << std::endl;
  }
}

bool WRTCServer::sendDataViaDataChannel(const std::string& data) {
  webrtc::DataBuffer buffer(data);
  sendDataViaDataChannel(buffer);
  return true;
}

bool WRTCServer::sendDataViaDataChannel(const webrtc::DataBuffer& buffer) {
  if (!data_channel.get()) {
    std::cout << "Data channel is not established" << std::endl;
    return false;
  }
  if (!data_channel->Send(buffer)) {
    switch (data_channel->state()) {
      case webrtc::DataChannelInterface::kConnecting:
        {
          std::cout << "Unable to send arraybuffer. DataChannel is connecting" << std::endl;
          break;
        }
      case webrtc::DataChannelInterface::kOpen:
        {
          std::cout << "Unable to send arraybuffer." << std::endl;
          break;
        }
      case webrtc::DataChannelInterface::kClosing:
        {
          std::cout << "Unable to send arraybuffer. DataChannel is closing" << std::endl;
          break;
        }
      case webrtc::DataChannelInterface::kClosed:
        {
          std::cout << "Unable to send arraybuffer. DataChannel is closed" << std::endl;
          break;
        }
      default:
        {
          std::cout << "Unable to send arraybuffer. DataChannel unknown state" << std::endl;
          break;
        }
    }
  }
  return true;
}

webrtc::DataChannelInterface::DataState WRTCServer::updateDataChannelState() {
  dataChannelstate = data_channel->state();
  return dataChannelstate;
}

void WRTCServer::InitAndRun() {
  std::cout << std::this_thread::get_id() << ":"
            << "WRTCServer::InitAndRun" << std::endl;
  // Create the PeerConnectionFactory.
  rtc::InitializeSSL();
  // TODO: free memory
  // TODO: reset(new rtc::Thread()) ???
  // SEE https://github.com/pristineio/webrtc-mirror/blob/7a5bcdffaab90a05bc1146b2b1ea71c004e54d71/webrtc/rtc_base/thread.cc

  //network_thread = rtc::Thread::CreateWithSocketServer();
  network_thread.reset(rtc::Thread::Current());//reset(new rtc::Thread());
  //network_thread = rtc::Thread::CreateWithSocketServer();//reset(new rtc::Thread());
  //network_thread = std::make_unique<rtc::Thread>(rtc::Thread::CreateWithSocketServer()); // rtc::Thread::CreateWithSocketServer();
  network_thread->SetName("network_thread 1", nullptr);

  worker_thread.reset(rtc::Thread::Current());//reset(new rtc::Thread());
  //worker_thread = rtc::Thread::Create();
  //worker_thread = rtc::Thread::Create();
  worker_thread->SetName("worker_thread 1", nullptr);

  signaling_thread.reset(rtc::Thread::Current());
  //signaling_thread = rtc::Thread::Create();
  signaling_thread->SetName("signaling_thread 1", nullptr);

  /*worker_thread->Invoke<bool>(RTC_FROM_HERE, [this]() {
    this->peer_connection_factory = webrtc::CreateModularPeerConnectionFactory(
    this->network_thread.get(), //rtc::Thread* network_thread,
    this->worker_thread.get(), //rtc::Thread* worker_thread,
    this->signaling_thread.get(), //rtc::Thread::Current(), //nullptr, //std::move(signaling_thread), //rtc::Thread* signaling_thread,
    nullptr, //std::unique_ptr<cricket::MediaEngineInterface> media_engine,
    nullptr, //std::unique_ptr<CallFactoryInterface> call_factory,
    nullptr //std::unique_ptr<RtcEventLogFactoryInterface> event_log_factory);
  );

    return true;
  });*/

  /*RTC_CHECK(network_thread->Start()) << "Failed to start thread";
  RTC_CHECK(worker_thread->Start()) << "Failed to start thread";
  RTC_CHECK(signaling_thread->Start()) << "Failed to start thread";*/

  /*
  * RTCPeerConnection that serves as the starting point
  * to create any type of connection, data channel or otherwise.
  **/
  peer_connection_factory = webrtc::CreateModularPeerConnectionFactory(
    network_thread.get(), //rtc::Thread* network_thread,
    worker_thread.get(), //rtc::Thread* worker_thread,
    signaling_thread.get(), //rtc::Thread::Current(), //nullptr, //std::move(signaling_thread), //rtc::Thread* signaling_thread,
    nullptr, //std::unique_ptr<cricket::MediaEngineInterface> media_engine,
    nullptr, //webrtc::CreateCallFactory(), //std::unique_ptr<CallFactoryInterface> call_factory,
    nullptr //webrtc::CreateRtcEventLogFactory() //std::unique_ptr<RtcEventLogFactoryInterface> event_log_factory);
  );
  std::cout << "Created PeerConnectionFactory" << std::endl;
  if (peer_connection_factory.get() == nullptr) {
    std::cout << "Error: Could not create CreatePeerConnectionFactory." << std::endl;
    // return?
  }
  /*webrtc::PeerConnectionFactoryInterface::Options webRtcOptions;
  // NOTE: you cannot disable encryption for SCTP-based data channels. And RTP-based data channels are not in the spec. 
  // SEE https://groups.google.com/forum/#!topic/discuss-webrtc/_6TCUy775PM
  webRtcOptions.disable_encryption = true;
  peer_connection_factory->SetOptions(webRtcOptions);*/
  //signaling_thread->set_socketserver(&socket_server);
  //signaling_thread.reset(new rtc::Thread(&socket_server));
  /*signaling_thread->Run();
  network_thread->Run();
  worker_thread->Run();*/
  /*if (!signaling_thread->Start()) {
    // TODO
  }
  if (!network_thread->Start()) {
    // TODO
  }
  if (!worker_thread->Start()) {
    // TODO
  }*/
  //signaling_thread->set_socketserver(nullptr);
}

void WRTCServer::resetWebRtcConfig(const std::vector<webrtc::PeerConnectionInterface::IceServer>& iceServers) {
  std::cout << std::this_thread::get_id() << ":"
            << "WRTCServer::resetWebRtcConfig" << std::endl;
  // settings for game-server messaging
  webrtc_gamedata_options.offer_to_receive_audio = false;
  webrtc_gamedata_options.offer_to_receive_video = false;

  // SCTP supports unordered data. Unordered data is unimportant for multiplayer games.
  data_channel_config.ordered = false;
  // maxRetransmits is 0, because if a message didn’t arrive, we don’t care.
  data_channel_config.maxRetransmits = 0;
  // data_channel_config.maxRetransmitTime = options.maxRetransmitTime; // TODO
  // TODO
  // data_channel_config.negotiated = true; // True if the channel has been externally negotiated
	// data_channel_config.id = 0;

  // set servers
  webrtcConfiguration.servers.clear();
  for (const auto& ice_server : iceServers) {
    // ICE is the protocol chosen for NAT traversal in WebRTC. 
    // SEE https://chromium.googlesource.com/external/webrtc/+/lkgr/pc/peerconnection.cc
    std::cout << "added ice_server " << ice_server.uri << std::endl;
    webrtcConfiguration.servers.push_back(ice_server);
    // If set to true, use RTP data channels instead of SCTP.
    // TODO(deadbeef): Remove this. We no longer commit to supporting RTP data
    // channels, though some applications are still working on moving off of
    // them.
    // RTP data channel rate limited! https://richard.to/programming/sending-images-with-webrtc-data-channels.html
    //webrtcConfiguration.enable_rtp_data_channel = true;
    //webrtcConfiguration.enable_rtp_data_channel = false;
    // Can be used to disable DTLS-SRTP. This should never be done, but can be
    // useful for testing purposes, for example in setting up a loopback call
    // with a single PeerConnection.
    //webrtcConfiguration.enable_dtls_srtp = false;
    //webrtcConfiguration.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
    //webrtcConfiguration.DtlsSrtpKeyAgreement
  }
}

bool WRTCServer::isDataChannelOpen() {
  return dataChannelstate == webrtc::DataChannelInterface::kOpen;
}

void WRTCServer::Quit() {
  // CloseDataChannel? https://webrtc.googlesource.com/src/+/master/examples/unityplugin/simple_peer_connection.cc
  network_thread->Quit();
  signaling_thread->Quit();
  worker_thread->Quit();
  //webrtc_thread.reset();
  rtc::CleanupSSL();
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
void WebRtcObserver::OnDataChannelMessage(const webrtc::DataBuffer& buffer) {
  std::cout << "OnDataChannelMessage" << std::endl;
  std::string data(buffer.data.data<char>(), buffer.data.size());
  std::cout << data << std::endl;
  webrtc::DataChannelInterface::DataState state = m_WRTC->data_channel->state();
  if (state != webrtc::DataChannelInterface::kOpen) {
    std::cout << "OnDataChannelMessage: data channel not open!" << std::endl;
    // TODO return false;  
  }
  // std::string str = "pong";
  // webrtc::DataBuffer resp(rtc::CopyOnWriteBuffer(str.c_str(), str.length()), false /* binary */);

  // send back?  
  m_WRTC->sendDataViaDataChannel(buffer);
}


// Callback for when the data channel is successfully created. We need to re-register the updated
// data channel here.
void WebRtcObserver::OnDataChannelCreated(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) {
  std::cout << "OnDataChannelCreated" << std::endl;
  m_WRTC->data_channel = channel;
  m_WRTC->data_channel->RegisterObserver(&data_channel_observer);
  //data_channel_count++; // NEED HERE?
}

// TODO: on closed

// Callback for when the STUN server responds with the ICE candidates.
// Sends by websocket JSON containing { candidate, sdpMid, sdpMLineIndex }
void WebRtcObserver::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
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
  m_WRTC->webSocketServer->send(payload);
}

void WebRtcObserver::onDataChannelOpen() {
  data_channel_count++;
  std::cout << "WebRtcObserver::onDataChannelOpen: data channel count: " << data_channel_count << std::endl;
}

void WebRtcObserver::onDataChannelClose() {
  data_channel_count--;
  std::cout << "WebRtcObserver::onDataChannelClose: data channel count: " << data_channel_count << std::endl;
}

// Callback for when the answer is created. This sends the answer back to the client.
void WebRtcObserver::OnAnswerCreated(webrtc::SessionDescriptionInterface* desc) {
  std::cout << "OnAnswerCreated" << std::endl;
  // store the server’s own answer
  m_WRTC->peer_connection->SetLocalDescription(&local_description_observer, desc);
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
  m_WRTC->webSocketServer->send(payload);
}

void CSDO::OnSuccess(webrtc::SessionDescriptionInterface* desc) {
  /*std::cout << std::this_thread::get_id() << ":"
            << "CreateSessionDescriptionObserver::OnSuccess" << std::endl;
  parent.onSuccessCSD(desc);*/
  m_observer->OnAnswerCreated(desc);
}

void CSDO::OnFailure(const std::string& error) {
  std::cout << std::this_thread::get_id() << ":"
            << "CreateSessionDescriptionObserver::OnFailure" << std::endl << error << std::endl;
}

// Change in state of the Data Channel.
void DCO::OnStateChange() {
  //m_observer->data_channel_count++;
  if (!m_observer->m_WRTC->data_channel) {
    std::cout << "DCO::OnStateChange: data channel empty!" << std::endl;
    return;
  }

  m_observer->m_WRTC->updateDataChannelState();
  webrtc::DataChannelInterface::DataState state = m_observer->m_WRTC->data_channel->state();

  switch (state) {
    case webrtc::DataChannelInterface::kConnecting:
      {
        std::cout << "DCO::OnStateChange: data channel connecting!" << std::endl;
        break;
      }
    case webrtc::DataChannelInterface::kOpen:
      {
        m_observer->onDataChannelOpen();
        std::cout << "DCO::OnStateChange: data channel open!" << std::endl;
        break;
      }
    case webrtc::DataChannelInterface::kClosing:
      {
        std::cout << "DCO::OnStateChange: data channel closing!" << std::endl;
        break;
      }
    case webrtc::DataChannelInterface::kClosed:
      {
        m_observer->onDataChannelClose();
        std::cout << "DCO::OnStateChange: data channel not open!" << std::endl;
        break;
      }
    default:
      {
        std::cout << "DCO::OnStateChange: unknown data channel state! " << state << std::endl;
      }
  }
}

void DCO::OnBufferedAmountChange(uint64_t /* previous_amount */) {}

// Message received.
void DCO::OnMessage(const webrtc::DataBuffer& buffer) {
  m_observer->OnDataChannelMessage(buffer);
}

// Triggered when a remote peer opens a data channel.
void PCO::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) {
  m_observer->OnDataChannelCreated(channel);
}

// Override ICE candidate.
void PCO::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
  m_observer->OnIceCandidate(candidate);
}

void PCO::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) {
  std::cout << std::this_thread::get_id() << ":"
            << "PeerConnectionObserver::SignalingChange(" << new_state << ")" << std::endl;
}

// Triggered when media is received on a new stream from remote peer.
void PCO::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface>/* stream*/) {
  std::cout << std::this_thread::get_id() << ":"
            << "PeerConnectionObserver::AddStream" << std::endl;
}

// Triggered when a remote peer close a stream.
void PCO::OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface>/* stream*/) {
  std::cout << std::this_thread::get_id() << ":"
            << "PeerConnectionObserver::RemoveStream" << std::endl;
}

// Triggered when renegotiation is needed. For example, an ICE restart
// has begun.
void PCO::OnRenegotiationNeeded() {
  std::cout << std::this_thread::get_id() << ":"
            << "PeerConnectionObserver::RenegotiationNeeded" << std::endl;
}

// Called any time the IceConnectionState changes.
//
// Note that our ICE states lag behind the standard slightly. The most
// notable differences include the fact that "failed" occurs after 15
// seconds, not 30, and this actually represents a combination ICE + DTLS
// state, so it may be "failed" if DTLS fails while ICE succeeds.
void PCO::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) {
  std::cout << std::this_thread::get_id() << ":"
            << "PeerConnectionObserver::IceConnectionChange(" << static_cast<int>(new_state) << ")" << std::endl;
  switch (new_state)
  {
    case webrtc::PeerConnectionInterface::kIceConnectionNew:
      {
        break;
      }
    case webrtc::PeerConnectionInterface::kIceConnectionChecking:
      {
        break;
      }
    case webrtc::PeerConnectionInterface::kIceConnectionConnected:
      break;
    case webrtc::PeerConnectionInterface::kIceConnectionCompleted:
      {
        break;
      }
    case webrtc::PeerConnectionInterface::kIceConnectionFailed:
      {
        break;
      }
    case webrtc::PeerConnectionInterface::kIceConnectionDisconnected:
      {
        break;
      }
    case webrtc::PeerConnectionInterface::kIceConnectionClosed:
      {
        break;
      }
    case webrtc::PeerConnectionInterface::kIceConnectionMax:
      // TODO
      /* not in https://developer.mozilla.org/en-US/docs/Web/API/RTCPeerConnection/iceConnectionState */
      {
        break;
      }
  }
}

// Called any time the IceGatheringState changes.
void PCO::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) {
  std::cout << std::this_thread::get_id() << ":"
            << "PeerConnectionObserver::IceGatheringChange(" << new_state << ")" << std::endl;
}
#include "net/webrtc/WRTCSession.hpp" // IWYU pragma: associated
#include "algorithm/DispatchQueue.hpp"
#include "algorithm/NetworkOperation.hpp"
#include "algorithm/StringUtils.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include "net/webrtc/Observers.hpp"
#include "net/webrtc/WRTCServer.hpp"
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

// TODO: prevent collision? respond ERROR to client if collided?
static std::string nextWrtcSessionId() { return utils::algo::genGuid(); }

// TODO: global?
// Constants for Session Description Protocol (SDP)
static const char kCandidateSdpMidName[] = "sdpMid";
static const char kCandidateSdpMlineIndexName[] = "sdpMLineIndex";
static const char kCandidateSdpName[] = "candidate";
static const char kAnswerSdpName[] = "answer";

webrtc::SessionDescriptionInterface*
createSessionDescriptionFromJson(const rapidjson::Document& message_object) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "createSessionDescriptionFromJson";
  webrtc::SdpParseError error;
  std::string sdp = message_object["payload"]["sdp"].GetString();
  LOG(INFO) << "sdp =" << sdp;
  // TODO: free memory?
  // TODO: handle error?
  webrtc::SessionDescriptionInterface* sdi = webrtc::CreateSessionDescription("offer", sdp, &error);
  if (sdi == nullptr) {
    LOG(WARNING) << "createSessionDescriptionFromJson:: SDI IS NULL" << error.description.c_str();
    LOG(WARNING) << error.description;
  }
  return sdi;
}

webrtc::IceCandidateInterface*
createIceCandidateFromJson(const rapidjson::Document& message_object) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "createIceCandidateFromJson";
  std::string candidate = message_object["payload"][kCandidateSdpName].GetString();
  LOG(INFO) << "got candidate from client =" << candidate;
  int sdp_mline_index = message_object["payload"][kCandidateSdpMlineIndexName].GetInt();
  std::string sdp_mid = message_object["payload"][kCandidateSdpMidName].GetString();
  webrtc::SdpParseError error;
  // TODO: free memory?
  // TODO: handle error?
  webrtc::IceCandidateInterface* iceCanidate =
      webrtc::CreateIceCandidate(sdp_mid, sdp_mline_index, candidate, &error);
  if (iceCanidate == nullptr) {
    LOG(WARNING) << "createIceCandidateFromJson:: iceCanidate IS NULL" << error.description.c_str();
    LOG(WARNING) << error.description;
  }
  return iceCanidate;
}
/*
WRTCSession::WRTCSession(NetworkManager* nm, std::string webrtcId, std::string wsId,
                         rtc::scoped_refptr<webrtc::PeerConnectionInterface> pci)
    : nm_(nm), pci_(pci), webrtcId_(webrtcId), wsId_(wsId),
      dataChannelstate_(webrtc::DataChannelInterface::kClosed) {
  // observers
  dataChannelObserver_ = std::make_unique<DCO>(nm, shared_from_this());
  createSDO_ = std::make_unique<CSDO>(nm, shared_from_this());
  localDescriptionObserver_ = std::make_unique<SSDO>(nm, shared_from_this());
  remoteDescriptionObserver_ = std::make_unique<SSDO>(nm, shared_from_this());
}*/

WRTCSession::WRTCSession(NetworkManager* nm, const std::string& webrtcId, const std::string& wsId)
    : nm_(nm), id_(webrtcId), wsId_(wsId),
      dataChannelstate_(webrtc::DataChannelInterface::kClosed) {
  receivedMessagesQueue_ =
      std::make_shared<algo::DispatchQueue>(std::string{"WebSockets Server Dispatch Queue"}, 0);
}

WRTCSession::~WRTCSession() {
  LOG(INFO) << "~WRTCSession";
  // nm_->getWRTC()->unregisterSession(id_);
}

void WRTCSession::CloseDataChannel(
    NetworkManager* nm, rtc::scoped_refptr<webrtc::DataChannelInterface>& in_data_channel,
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> pci) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "CloseDataChannel";

  {
    rtc::CritScope lock(&nm->getWRTC()->pcMutex_);

    if (!in_data_channel || !in_data_channel.get()) {
      LOG(WARNING) << "CloseDataChannel: empty in_data_channel";
      return;
    }

    if (in_data_channel.get()) {
      in_data_channel->UnregisterObserver();
      if (in_data_channel->state() == webrtc::DataChannelInterface::kClosed) {
        LOG(WARNING) << "CloseDataChannel: DataChannelInterface already closed";
        /////return;
      } else {
        in_data_channel->Close();
        in_data_channel.release();
      }
    }
    // in_data_channel = nullptr;

    // TODO
    // pci_ = nullptr;

    if (pci) {
      pci->Close();
      pci.release();
    }
  }
}

void WRTCSession::setObservers() {
  // NOTE: shared_from_this can't be used in constructor
  // observers
  LOG(INFO) << "creating dataChannelObserver_ ";
  dataChannelObserver_ = std::make_unique<DCO>(nm_, shared_from_this());
  if (!dataChannelObserver_ || !dataChannelObserver_.get()) {
    LOG(WARNING) << "empty dataChannelObserver_";
    return;
  }

  LOG(INFO) << "creating CSDO ";
  createSDO_ = std::make_unique<CSDO>(nm_, shared_from_this());
  if (!createSDO_ || !createSDO_.get()) {
    LOG(WARNING) << "empty CSDO";
    return;
  }

  LOG(INFO) << "creating localDescriptionObserver_ ";
  localDescriptionObserver_ = std::make_unique<SSDO>(nm_, shared_from_this());
  if (!localDescriptionObserver_ || !localDescriptionObserver_.get()) {
    LOG(WARNING) << "empty localDescriptionObserver_";
    return;
  }

  LOG(INFO) << "creating remoteDescriptionObserver_ ";
  remoteDescriptionObserver_ = std::make_unique<SSDO>(nm_, shared_from_this());
  if (!remoteDescriptionObserver_ || !remoteDescriptionObserver_.get()) {
    LOG(WARNING) << "empty remote_description_observer";
    return;
  }
}
/*
rtc::scoped_refptr<webrtc::PeerConnectionInterface> WRTCSession::getPCI() const {
  return pci_;
}*/

/*rtc::scoped_refptr<webrtc::DataChannelInterface> WRTCSession::getDataChannelI() const {
  return dataChannelI_;
}*/

void WRTCSession::send(std::shared_ptr<std::string> ss) { send(ss.get()->c_str()); }

void WRTCSession::send(const std::string& data) {
  WRTCSession::sendDataViaDataChannel(nm_, shared_from_this(), data);
}

bool WRTCSession::sendDataViaDataChannel(NetworkManager* nm, std::shared_ptr<WRTCSession> wrtcSess,
                                         const std::string& data) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::sendDataViaDataChannel";

  if (!wrtcSess) {
    LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: wrtc session is not established";
    return false;
  }

  if (!wrtcSess->isDataChannelOpen()) {
    LOG(WARNING) << "sendDataViaDataChannel: dataChannel not open!";
    return false;
  }

  webrtc::DataBuffer buffer(data);
  sendDataViaDataChannel(nm, wrtcSess, buffer);
  return true;
}

bool WRTCSession::sendDataViaDataChannel(NetworkManager* nm, std::shared_ptr<WRTCSession> wrtcSess,
                                         const webrtc::DataBuffer& buffer) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::sendDataViaDataChannel";

  if (!wrtcSess) {
    LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: wrtc session is not established";
    return false;
  }

  if (!wrtcSess->isDataChannelOpen()) {
    LOG(WARNING) << "sendDataViaDataChannel: dataChannel not open!";
    return false;
  }

  if (!wrtcSess->dataChannelI_ || !wrtcSess->dataChannelI_.get()) {
    LOG(WARNING) << "sendDataViaDataChannel: empty peer_connection!";
    return false;
  }

  if (!wrtcSess->dataChannelI_->Send(buffer)) {
    switch (wrtcSess->dataChannelI_->state()) {
    case webrtc::DataChannelInterface::kConnecting: {
      LOG(WARNING)
          << "WRTCSession::sendDataViaDataChannel: Unable to send arraybuffer. DataChannel "
             "is connecting";
      break;
    }
    case webrtc::DataChannelInterface::kOpen: {
      LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: Unable to send arraybuffer.";
      break;
    }
    case webrtc::DataChannelInterface::kClosing: {
      LOG(WARNING)
          << "WRTCSession::sendDataViaDataChannel: Unable to send arraybuffer. DataChannel "
             "is closing";
      break;
    }
    case webrtc::DataChannelInterface::kClosed: {
      LOG(WARNING)
          << "WRTCSession::sendDataViaDataChannel: Unable to send arraybuffer. DataChannel "
             "is closed";
      break;
    }
    default: {
      LOG(WARNING)
          << "WRTCSession::sendDataViaDataChannel: Unable to send arraybuffer. DataChannel "
             "unknown state";
      break;
    }
    }
  }
  return true;
}

webrtc::DataChannelInterface::DataState WRTCSession::updateDataChannelState() {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::updateDataChannelState";

  if (!dataChannelI_ || !dataChannelI_.get()) {
    LOG(WARNING) << "WRTCSession::updateDataChannelState: Data channel is not established";
    return webrtc::DataChannelInterface::kClosed;
  }

  dataChannelstate_ = dataChannelI_->state();
  return dataChannelstate_;
}

void WRTCSession::setLocalDescription(webrtc::SessionDescriptionInterface* sdi) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::setLocalDescription";

  {
    rtc::CritScope lock(&nm_->getWRTC()->pcMutex_);
    if (!localDescriptionObserver_) {
      LOG(WARNING) << "empty local_description_observer";
      return;
    }

    if (!pci_ || !pci_.get()) {
      LOG(WARNING) << "setLocalDescription: empty peer_connection!";
      return;
    }

    // store the server’s own answer
    pci_->SetLocalDescription(localDescriptionObserver_.get(), sdi);
  }
  // setLocalDescription(&local_description_observer, sdi);
}

void WRTCSession::createAndAddIceCandidate(const rapidjson::Document& message_object) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::createAndAddIceCandidate";
  // rtc::CritScope lock(&pc_mutex_);
  auto candidate_object = createIceCandidateFromJson(message_object);
  {
    rtc::CritScope lock(&nm_->getWRTC()->pcMutex_);

    if (!pci_ || !pci_.get()) {
      LOG(WARNING) << "createAndAddIceCandidate: empty peer_connection!";
      return;
    }

    // sends IceCandidate to Client via websockets, see OnIceCandidate
    pci_->AddIceCandidate(candidate_object);
  }
}

bool WRTCSession::isDataChannelOpen() {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::isDataChannelOpen";
  if (!dataChannelI_ || !dataChannelI_.get()) {
    LOG(WARNING) << "WRTCSession::isDataChannelOpen: Data channel is not established";
    return webrtc::DataChannelInterface::kClosed;
  }

  return dataChannelstate_ == webrtc::DataChannelInterface::kOpen;
}

void WRTCSession::createDCI() {
  LOG(INFO) << "creating DataChannel...";
  const std::string data_channel_lable = "dc";
  dataChannelI_ = pci_->CreateDataChannel(data_channel_lable, &nm_->getWRTC()->dataChannelConf_);
  LOG(INFO) << "created DataChannel";
  LOG(INFO) << "registering observer...";
  if (!dataChannelObserver_) {
    LOG(WARNING) << "empty data_channel_observer";
    return;
  }

  if (!dataChannelI_ || !dataChannelI_.get()) {
    LOG(WARNING) << "empty dataChannelI_";
    return;
  }

  dataChannelI_->RegisterObserver(dataChannelObserver_.get());
  LOG(INFO) << "registered observer";
}

void WRTCSession::SetRemoteDescription(
    webrtc::SessionDescriptionInterface* clientSessionDescription) {
  LOG(INFO) << "SetRemoteDescription...";

  {
    rtc::CritScope lock(&nm_->getWRTC()->pcMutex_);
    LOG(INFO) << "pc_mutex_...";
    if (!pci_) {
      LOG(WARNING) << "empty peer_connection!";
    }

    if (!remoteDescriptionObserver_) {
      LOG(WARNING) << "empty remote_description_observer";
    }

    pci_->SetRemoteDescription(remoteDescriptionObserver_.get(), clientSessionDescription);
  }
}

void WRTCSession::CreateAnswer() {
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
    rtc::CritScope lock(&nm_->getWRTC()->pcMutex_);
    if (!createSDO_) {
      LOG(WARNING) << "empty create_session_description_observer";
    }
    pci_->CreateAnswer(createSDO_.get(), nm_->getWRTC()->webrtcGamedataOpts_);
  }
  LOG(INFO) << "peer_connection created answer";
}

void WRTCSession::setRemoteDescriptionAndCreateAnswer(WsSession* clientWsSession,
                                                      NetworkManager* nm,
                                                      const rapidjson::Document& message_object) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCServer::SetRemoteDescriptionAndCreateAnswer";

  std::shared_ptr<WRTCSession> createdWRTCSession;

  // std::unique_ptr<cricket::PortAllocator> port_allocator(new
  // cricket::BasicPortAllocator(new rtc::BasicNetworkManager()));
  // port_allocator->SetPortRange(60000, 60001);
  // TODO ice_server.username = "xxx";
  // TODO ice_server.password = kTurnPassword;
  LOG(INFO) << "creating peer_connection...";
  {
    const std::string& webrtcConnId = nextWrtcSessionId();
    const std::string& wsConnId = clientWsSession->getId();

    LOG(INFO) << "creating peerConnectionObserver...";
    clientWsSession->peerConnectionObserver_ =
        std::make_shared<PCO>(nm, webrtcConnId, wsConnId); // TODO: to private

    if (!clientWsSession->peerConnectionObserver_) {
      LOG(WARNING) << "empty peer_connection_observer";
    }

    rtc::CritScope lock(&nm->getWRTC()->pcMutex_);
    auto newPeerConn = nm->getWRTC()->getPCF()->CreatePeerConnection(
        nm->getWRTC()->getWRTCConf(), std::move(nm->getWRTC()->portAllocator_), nullptr,
        clientWsSession->peerConnectionObserver_.get());
    if (!newPeerConn) {
      LOG(WARNING) << "_pcfactory->CreatePeerConnection() failed!";
    }
    LOG(INFO) << "creating WRTCSession...";
    {
      createdWRTCSession = std::make_shared<WRTCSession>(nm, webrtcConnId, wsConnId);
      auto isSessCreated = nm->getWRTC()->addSession(webrtcConnId, createdWRTCSession);
      if (!isSessCreated) {
        LOG(WARNING) << "setRemoteDescriptionAndCreateAnswer: Can`t create session ";
        return;
      }
      createdWRTCSession->pci_ = newPeerConn; // prevents garbage collection by 'operator='
      if (!createdWRTCSession->pci_ || !createdWRTCSession->pci_.get()) {
        LOG(WARNING) << "WRTCSession::setRemoteDescriptionAndCreateAnswer: empty PCI";
      }
      clientWsSession->pairToWRTCSession(createdWRTCSession);
      createdWRTCSession->setObservers();
      LOG(INFO) << "updating peerConnections_ for webrtcConnId = " << webrtcConnId;
    }
  }

  createdWRTCSession->createDCI();

  //
  LOG(INFO) << "createSessionDescriptionFromJson...";
  auto clientSessionDescription = createSessionDescriptionFromJson(message_object);
  if (!clientSessionDescription) {
    LOG(WARNING) << "empty clientSessionDescription!";
  }

  createdWRTCSession->SetRemoteDescription(clientSessionDescription);

  createdWRTCSession->CreateAnswer();
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
void WRTCSession::onDataChannelMessage(const webrtc::DataBuffer& buffer) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::OnDataChannelMessage";
  std::string data(buffer.data.data<char>(), buffer.data.size());
  LOG(INFO) << data;

  if (!dataChannelI_ || !dataChannelI_.get()) {
    LOG(WARNING) << "onDataChannelMessage: Invalid dataChannelI_";
    return;
  }

  webrtc::DataChannelInterface::DataState state = dataChannelI_->state();
  if (state != webrtc::DataChannelInterface::kOpen) {
    LOG(WARNING) << "OnDataChannelMessage: data channel not open!";
    // TODO return false;
  }
  // std::string str = "pong";
  // webrtc::DataBuffer resp(rtc::CopyOnWriteBuffer(str.c_str(), str.length()),
  // false /* binary */);

  handleIncomingJSON(buffer);

  // send back?
  // WRTCSession::sendDataViaDataChannel(nm_, shared_from_this(), buffer);
}

/**
 * Add message to queue for further processing
 * Returs true if message can be processed
 **/
bool WRTCSession::handleIncomingJSON(const webrtc::DataBuffer& buffer) {
  std::shared_ptr<std::string> incomingStr =
      std::make_shared<std::string>(std::string((char*)buffer.data.data(), buffer.data.size()));
  // parse incoming message
  rapidjson::Document message_object;
  rapidjson::ParseResult result = message_object.Parse(incomingStr.get()->c_str());
  LOG(INFO) << "incomingStr: " << incomingStr.get()->c_str();
  if (!result || !message_object.IsObject() || !message_object.HasMember("type")) {
    LOG(WARNING) << "WRTCSession::on_read: ignored invalid message without type";
    return false;
  }
  // Probably should do some error checking on the JSON object.
  std::string typeStr = message_object["type"].GetString();
  if (typeStr.empty() || typeStr.length() > UINT32_FIELD_MAX_LEN) {
    LOG(WARNING) << "WRTCSession::on_read: ignored invalid message with invalid "
                    "type field";
  }
  const auto& callbacks = nm_->getWRTC()->getOperationCallbacks().getCallbacks();

  const WRTCNetworkOperation wrtcNetworkOperation =
      static_cast<algo::WRTC_OPCODE>(algo::Opcodes::wrtcOpcodeFromStr(typeStr));
  const auto itFound = callbacks.find(wrtcNetworkOperation);
  // if a callback is registered for event, add it to queue
  if (itFound != callbacks.end()) {
    WRTCNetworkOperationCallback callback = itFound->second;
    algo::DispatchQueue::dispatch_callback callbackBind =
        std::bind(callback, this, nm_, incomingStr);
    if (!receivedMessagesQueue_ || !receivedMessagesQueue_.get()) {
      LOG(WARNING) << "WsSession::handleIncomingJSON: invalid receivedMessagesQueue_ ";
      return false;
    }
    receivedMessagesQueue_->dispatch(callbackBind);
    // callbackBind();
  } else {
    LOG(WARNING) << "WsSession::handleIncomingJSON: ignored invalid message with type " << typeStr;
    return false;
  }

  return true;
}

// Callback for when the data channel is successfully created. We need to
// re-register the updated data channel here.
void WRTCSession::onDataChannelCreated(NetworkManager* nm, std::shared_ptr<WRTCSession> wrtcSess,
                                       rtc::scoped_refptr<webrtc::DataChannelInterface> channel) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::OnDataChannelCreated";

  if (!nm) {
    LOG(WARNING) << "onDataChannelCreated: Invalid NetworkManager";
    return;
  }

  if (!channel) {
    LOG(WARNING) << "onDataChannelCreated: Invalid DataChannelInterface";
    return;
  }

  if (!wrtcSess) {
    LOG(WARNING) << std::this_thread::get_id() << ":"
                 << "WRTCSession::OnDataChannelCreated: INVALID wrtcSess!";
    return;
  }

  wrtcSess->dataChannelI_ = channel;
  if (!wrtcSess->dataChannelObserver_) {
    LOG(WARNING) << std::this_thread::get_id() << ":"
                 << "empty data_channel_observer";
    return;
  }

  if (!wrtcSess->dataChannelI_ || !wrtcSess->dataChannelI_.get()) {
    LOG(WARNING) << "onDataChannelCreated: Invalid dataChannelI_";
    return;
  }

  wrtcSess->dataChannelI_->RegisterObserver(wrtcSess->dataChannelObserver_.get());
  // data_channel_count++; // NEED HERE?
}

// TODO: on closed

// Callback for when the STUN server responds with the ICE candidates.
// Sends by websocket JSON containing { candidate, sdpMid, sdpMLineIndex }
// TODO: WORKS WITHOUT OnIceCandidate???
void WRTCSession::onIceCandidate(NetworkManager* nm, const std::string& wsConnId,
                                 const webrtc::IceCandidateInterface* candidate) {
  const std::string sdp_mid_copy = candidate->sdp_mid();

  if (!nm) {
    LOG(WARNING) << "onIceCandidate: Invalid NetworkManager";
    return;
  }

  if (!candidate) {
    LOG(WARNING) << "onIceCandidate: Invalid IceCandidateInterface";
    return;
  }

  auto sess = nm->getWS()->getSessById(wsConnId);
  if (!sess || !sess.get()) {
    LOG(WARNING) << "onIceCandidate: Invalid getSessById for " << wsConnId;
    return;
  }

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::OnIceCandidate";
  std::string candidate_str;
  if (!candidate->ToString(&candidate_str)) {
    LOG(WARNING) << "Failed to serialize candidate";
    // LOG(LS_ERROR) << "Failed to serialize candidate";
    return;
  }
  candidate->ToString(&candidate_str);
  rapidjson::Document message_object;
  message_object.SetObject();
  message_object.AddMember(
      "type", rapidjson::StringRef(algo::Opcodes::opcodeToStr(algo::WS_OPCODE::CANDIDATE)),
      message_object.GetAllocator());
  rapidjson::Value candidate_value;
  candidate_value.SetString(rapidjson::StringRef(candidate_str.c_str()));
  rapidjson::Value sdp_mid_value;
  sdp_mid_value.SetString(rapidjson::StringRef(sdp_mid_copy.c_str()));
  rapidjson::Value message_payload;
  message_payload.SetObject();
  // candidate: Host candidate for RTP on UDP - in this ICE line our browser is
  // giving its host candidates- the IP example "candidate":
  // "candidate:1791031112 1 udp 2122260223 10.10.15.25 57339 typ host
  // generation 0 ufrag say/ network-id 1 network-cost 10",
  message_payload.AddMember(kCandidateSdpName, candidate_value, message_object.GetAllocator());
  // sdpMid: audio or video or ...
  message_payload.AddMember(kCandidateSdpMidName, sdp_mid_value, message_object.GetAllocator());
  message_payload.AddMember(kCandidateSdpMlineIndexName, candidate->sdp_mline_index(),
                            message_object.GetAllocator());
  message_object.AddMember("payload", message_payload, message_object.GetAllocator());
  rapidjson::StringBuffer strbuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  bool done = message_object.Accept(writer);
  if (!done) {
    LOG(WARNING) << "OnIceCandidate: INVALID JSON!";
    return;
  }
  std::string payload = strbuf.GetString();

  if (sess)
    sess->send(payload);
}

// Callback for when the answer is created. This sends the answer back to the
// client.
void WRTCSession::onAnswerCreated(webrtc::SessionDescriptionInterface* sdi) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::OnAnswerCreated";
  if (sdi == nullptr) {
    LOG(WARNING) << "WRTCSession::OnAnswerCreated INVALID SDI";
    return;
  }

  auto sess = nm_->getWS()->getSessById(wsId_);
  if (!sess || !sess.get()) {
    LOG(WARNING) << "onAnswerCreated: Invalid getSessById for " << wsId_;
    return;
  }

  LOG(INFO) << "OnAnswerCreated";
  // store the server’s own answer
  setLocalDescription(sdi);
  // TODO: replace rapidjson to ...?
  std::string offer_string;
  sdi->ToString(&offer_string);
  rapidjson::Document message_object;
  message_object.SetObject();
  rapidjson::Value type;
  message_object.AddMember(
      "type", rapidjson::StringRef(algo::Opcodes::opcodeToStr(algo::WS_OPCODE::ANSWER)),
      message_object.GetAllocator());
  rapidjson::Value sdp_value;
  sdp_value.SetString(rapidjson::StringRef(offer_string.c_str()));

  rapidjson::Value message_payload;
  message_payload.SetObject();
  message_payload.AddMember(
      "type",
      kAnswerSdpName, // rapidjson::StringRef(ANSWER_OPERATION.operationCodeStr_.c_str()),
      message_object.GetAllocator());
  message_payload.AddMember("sdp", sdp_value, message_object.GetAllocator());
  message_object.AddMember("payload", message_payload, message_object.GetAllocator());

  rapidjson::StringBuffer strbuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  bool done = message_object.Accept(writer);
  if (!done) {
    LOG(WARNING) << "OnAnswerCreated: INVALID JSON!";
    return;
  }
  std::string payload = strbuf.GetString();

  if (sess)
    sess->send(payload);
}

std::shared_ptr<algo::DispatchQueue> WRTCSession::getReceivedMessages() const {
  // NOTE: Returned smart pointer by value to increment reference count
  return receivedMessagesQueue_;
}

bool WRTCSession::hasReceivedMessages() const {
  if (!receivedMessagesQueue_) {
    LOG(WARNING) << "WsSession::hasReceivedMessages invalid receivedMessagesQueue_";
    return true;
  }
  return receivedMessagesQueue_.get()->isEmpty();
}

void WRTCSession::onDataChannelOpen() {
  LOG(INFO) << "WRTCSession::onDataChannelOpen";
  nm_->getWRTC()->addDataChannelCount(1);
}

void WRTCSession::onDataChannelClose() {
  LOG(INFO) << "WRTCSession::onDataChannelClose";
  nm_->getWRTC()->subDataChannelCount(1);
}

} // namespace net
} // namespace utils

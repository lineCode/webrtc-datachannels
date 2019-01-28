#include "net/webrtc/WRTCSession.hpp" // IWYU pragma: associated
#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
#include "algo/StringUtils.hpp"
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

namespace {

// Constants for Session Description Protocol (SDP)
static const char kCandidateSdpMidName[] = "sdpMid";
static const char kCandidateSdpMlineIndexName[] = "sdpMLineIndex";
static const char kCandidateSdpName[] = "candidate";
static const char kAnswerSdpName[] = "answer";

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

} // namespace

namespace gloer {
namespace net {

const boost::posix_time::time_duration WRTCSession::timerDeadlinePeriod =
    boost::posix_time::seconds(60);

WRTCSession::WRTCSession(NetworkManager* nm, const std::string& webrtcId, const std::string& wsId)
    : SessionBase(webrtcId), dataChannelstate_(webrtc::DataChannelInterface::kClosed), nm_(nm),
      wsId_(wsId) {
  receivedMessagesQueue_ =
      std::make_shared<algo::DispatchQueue>(std::string{"WebSockets Server Dispatch Queue"}, 0);
}

WRTCSession::~WRTCSession() {
  LOG(INFO) << "~WRTCSession";
  /*if (receivedMessagesQueue_ && receivedMessagesQueue_.get())
    receivedMessagesQueue_.reset();*/
  CloseDataChannel(nm_, dataChannelI_, pci_);
  //nm_->getWRTC()->unregisterSession(id_);
  // TODO: move timerDeadline logic to webrtc thread
}

void WRTCSession::CloseDataChannel(
    NetworkManager* nm, rtc::scoped_refptr<webrtc::DataChannelInterface>& in_data_channel,
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> pci) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "CloseDataChannel";

  {
    LOG(INFO) << std::this_thread::get_id() << ":"
              << "WRTCSession::CloseDataChannel peerConIMutex_";
    // rtc::CritScope lock(&peerConIMutex_);

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

    /*if (sess && sess.get()) {
      LOG(WARNING) << "WRTCServer::unregisterSession: clean observers...";
      sess->updateDataChannelState();
      if (sess->localDescriptionObserver_) {
        // sess->localDescriptionObserver_->Release();
        sess->localDescriptionObserver_.release();
      }
      if (sess->remoteDescriptionObserver_) {
        // sess->remoteDescriptionObserver_->Release();
        sess->remoteDescriptionObserver_.release();
      }
      if (sess->createSDO_) {
        // sess->createSDO_->Release();
        sess->createSDO_.release();
      }
      if (sess->dataChannelObserver_) {
        // sess->dataChannelObserver_->Release();
        // sess->dataChannelObserver_.release();
        sess->dataChannelObserver_.reset();
      }
    }*/

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
  createSDO_ = new rtc::RefCountedObject<CSDO>(nm_, shared_from_this());
  if (!createSDO_ || !createSDO_.get()) {
    LOG(WARNING) << "empty CSDO";
    return;
  }

  LOG(INFO) << "creating localDescriptionObserver_ ";
  localDescriptionObserver_ = new rtc::RefCountedObject<SSDO>(nm_, shared_from_this());
  if (!localDescriptionObserver_ || !localDescriptionObserver_.get()) {
    LOG(WARNING) << "empty localDescriptionObserver_";
    return;
  }

  LOG(INFO) << "creating remoteDescriptionObserver_ ";
  remoteDescriptionObserver_ = new rtc::RefCountedObject<SSDO>(nm_, shared_from_this());
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

void WRTCSession::send(std::shared_ptr<std::string> ss) {
  if (!ss || !ss.get()) {
    LOG(WARNING) << "WRTCSession::send: Invalid messageBuffer";
    return;
  }
  // send(*ss.get());
  send(ss.get()->c_str());
}

void WRTCSession::send(const std::string& data) {
  /*LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::sendDataViaDataChannel const std::string&";*/
  WRTCSession::sendDataViaDataChannel(nm_, shared_from_this(), data);
}

bool WRTCSession::sendDataViaDataChannel(NetworkManager* nm, std::shared_ptr<WRTCSession> wrtcSess,
                                         const std::string& data) {
  /*LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::sendDataViaDataChannel std::shared_ptr<WRTCSession> wrtcSess, const "
               "std::string& data";*/

  if (!wrtcSess) {
    LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: wrtc session is not established";
    return false;
  }

  if (!wrtcSess->isOpen()) {
    LOG(WARNING) << "sendDataViaDataChannel: dataChannel not open!";
    return false;
  }

  webrtc::DataBuffer buffer(data);
  sendDataViaDataChannel(nm, wrtcSess, buffer);
  return true;
}

bool WRTCSession::sendDataViaDataChannel(NetworkManager* nm, std::shared_ptr<WRTCSession> wrtcSess,
                                         const webrtc::DataBuffer& buffer) {

  /*LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::sendDataViaDataChannel const webrtc::DataBuffer& buffer";*/

  if (!buffer.size()) {
    LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: Invalid messageBuffer";
    return false;
  }

  if (buffer.size() > maxSendMsgSizebyte) {
    LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: Too big messageBuffer of size "
                 << buffer.size();
    return false;
  }

  if (!wrtcSess) {
    LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: wrtc session is not established";
    return false;
  }

  if (!wrtcSess->isOpen()) {
    LOG(WARNING) << "sendDataViaDataChannel: dataChannel not open!";
    return false;
  }

  if (!wrtcSess->dataChannelI_ || !wrtcSess->dataChannelI_.get()) {
    LOG(WARNING) << "sendDataViaDataChannel: empty peer_connection!";
    return false;
  }

  /*LOG(INFO) << std::this_thread::get_id() << ":"
            << "wrtcSess->dataChannelI_->Send " << buffer.size();*/

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
    /*LOG(INFO) << std::this_thread::get_id() << ":"
              << "WRTCSession::setLocalDescription peerConIMutex_";*/
    // rtc::CritScope lock(&peerConIMutex_);
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
  auto candidate_object = createIceCandidateFromJson(message_object);
  {
    LOG(INFO) << std::this_thread::get_id() << ":"
              << "WRTCSession::createAndAddIceCandidate peerConIMutex_";
    // rtc::CritScope lock(&peerConIMutex_);

    if (!pci_ || !pci_.get()) {
      LOG(WARNING) << "createAndAddIceCandidate: empty peer_connection!";
      return;
    }

    // sends IceCandidate to Client via websockets, see OnIceCandidate
    pci_->AddIceCandidate(candidate_object);
  }
}

bool WRTCSession::isOpen() {
  /*LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::isDataChannelOpen";*/
  if (!dataChannelI_ || !dataChannelI_.get()) {
    LOG(WARNING) << "WRTCSession::isDataChannelOpen: Data channel is not established";
    return webrtc::DataChannelInterface::kClosed;
  }

  return dataChannelstate_ == webrtc::DataChannelInterface::kOpen;
}

void WRTCSession::createDCI() {
  LOG(INFO) << "creating DataChannel...";
  const std::string data_channel_lable = "dc_" + getId();

  {
    LOG(INFO) << std::this_thread::get_id() << ":"
              << "WRTCSession::createDCI peerConIMutex_";
    // rtc::CritScope lock(&peerConIMutex_);
    dataChannelI_ = pci_->CreateDataChannel(data_channel_lable, &nm_->getWRTC()->dataChannelConf_);
  }
  LOG(INFO) << "created DataChannel";
  LOG(INFO) << "registering observer...";
  if (!dataChannelObserver_ || !dataChannelObserver_.get()) {
    LOG(WARNING) << "empty data_channel_observer";
    return;
  }

  if (!dataChannelI_ || !dataChannelI_.get()) {
    LOG(WARNING) << "empty dataChannelI_";
    return;
  }

  // Used to receive events from the data channel. Only one observer can be
  // registered at a time. UnregisterObserver should be called before the
  // observer object is destroyed.
  dataChannelI_->RegisterObserver(dataChannelObserver_.get());
  LOG(INFO) << "registered observer";
}

void WRTCSession::SetRemoteDescription(
    webrtc::SessionDescriptionInterface* clientSessionDescription) {
  LOG(INFO) << "SetRemoteDescription...";

  {
    LOG(INFO) << std::this_thread::get_id() << ":"
              << "WRTCSession::SetRemoteDescription peerConIMutex_";

    if (!remoteDescriptionObserver_) {
      LOG(WARNING) << "empty remote_description_observer";
    }
    {
      // rtc::CritScope lock(&peerConIMutex_);
      LOG(INFO) << "pc_mutex_...";
      if (!pci_) {
        LOG(WARNING) << "empty peer_connection!";
      }

      pci_->SetRemoteDescription(remoteDescriptionObserver_.get(), clientSessionDescription);
    }
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
    LOG(INFO) << std::this_thread::get_id() << ":"
              << "WRTCSession::CreateAnswer peerConIMutex_";
    // rtc::CritScope lock(&peerConIMutex_);
    if (!createSDO_ || !createSDO_.get() || !nm_ || !nm_->getWRTC() || !nm_->getWRTC().get()) {
      LOG(WARNING) << "empty create_session_description_observer";
      return;
    }
    // The CreateSessionDescriptionObserver callback will be called when done.
    pci_->CreateAnswer(createSDO_.get(), nm_->getWRTC()->webrtcGamedataOpts_);
  }
  // LOG(INFO) << "peer_connection created answer";

  isFullyCreated_ = true; // TODO
}

/*
 * TODO: Use Facebook’s lock-free queue && process the network messages in batch
 * server also contains a glaring inefficiency in its
 * immediate handling of data channel messages in the OnDataChannelMessage
 * callback. The cost of doing so in our example is negligible, but in an actual
 * game server, the message handler will be a more costly function that must
 * interact with state. The message handling function will then block the
 * signaling thread from processing any other messages on the wire during its
 * execution. To avoid this, I recommend pushing all messages onto a thread-safe
 * message queue, and during the next game tick, the main game loop running in a
 * different thread can process the network messages in batch.
 **/
// Callback for when the server receives a message on the data channel.
void WRTCSession::onDataChannelMessage(const webrtc::DataBuffer& buffer) {
  /*LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::OnDataChannelMessage";*/

  lastRecievedMsgTime = boost::posix_time::second_clock::local_time();
  timerDeadline = lastRecievedMsgTime + timerDeadlinePeriod;

  if (!buffer.size()) {
    LOG(WARNING) << "WRTCSession::onDataChannelMessage: Invalid messageBuffer";
    return;
  }

  if (buffer.size() > maxReceiveMsgSizebyte) {
    LOG(WARNING) << "WRTCSession::onDataChannelMessage: Too big messageBuffer of size "
                 << buffer.size();
    return;
  }

  const std::shared_ptr<std::string> data =
      std::make_shared<std::string>(std::string(buffer.data.data<char>(), buffer.size()));

  if (!data || !data.get()) {
    LOG(WARNING) << "WRTCSession::handleIncomingJSON: invalid message";
    return;
  }
  LOG(INFO) << data->c_str();

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

  handleIncomingJSON(data);

  // send back?
  // WRTCSession::sendDataViaDataChannel(nm_, shared_from_this(), buffer);
}

/**
 * Add message to queue for further processing
 * Returs true if message can be processed
 **/
bool WRTCSession::handleIncomingJSON(std::shared_ptr<std::string> message) {
  if (!message || !message.get()) {
    LOG(WARNING) << "WRTCSession::handleIncomingJSON: invalid message";
    return false;
  }

  // parse incoming message
  rapidjson::Document message_object;
  rapidjson::ParseResult result = message_object.Parse(message->c_str());
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
  const auto& callbacks = nm_->getWRTC()->getOperationCallbacks().getCallbacks();

  const WRTCNetworkOperation wrtcNetworkOperation =
      static_cast<algo::WRTC_OPCODE>(algo::Opcodes::wrtcOpcodeFromStr(typeStr));
  const auto itFound = callbacks.find(wrtcNetworkOperation);
  // if a callback is registered for event, add it to queue
  if (itFound != callbacks.end()) {
    WRTCNetworkOperationCallback callback = itFound->second;
    algo::DispatchQueue::dispatch_callback callbackBind = std::bind(callback, this, nm_, message);
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

  // Used to receive events from the data channel. Only one observer can be
  // registered at a time. UnregisterObserver should be called before the
  // observer object is destroyed.
  wrtcSess->dataChannelI_->RegisterObserver(wrtcSess->dataChannelObserver_.get());

  wrtcSess->updateDataChannelState();
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

  auto wsSess = nm->getWS()->getSessById(wsConnId);
  if (!wsSess || !wsSess.get()) {
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

  if (wsSess)
    wsSess->send(payload);
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

  auto wsSess = nm_->getWS()->getSessById(wsId_);
  if (!wsSess || !wsSess.get()) {
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

  if (wsSess && wsSess.get() && wsSess->isOpen())
    wsSess->send(payload);
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
} // namespace gloer

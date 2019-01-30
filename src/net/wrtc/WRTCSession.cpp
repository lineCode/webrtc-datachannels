#include "net/wrtc/WRTCSession.hpp" // IWYU pragma: associated
#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
#include "algo/StringUtils.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include "net/wrtc/Observers.hpp"
#include "net/wrtc/PeerConnectivityChecker.h"
#include "net/wrtc/WRTCServer.hpp"
#include "net/ws/WsServer.hpp"
#include "net/ws/WsSession.hpp"
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

std::unique_ptr<webrtc::IceCandidateInterface>
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
  std::unique_ptr<webrtc::IceCandidateInterface> iceCanidate(
      webrtc::CreateIceCandidate(sdp_mid, sdp_mline_index, candidate, &error));
  if (!iceCanidate.get()) {
    LOG(WARNING) << "createIceCandidateFromJson:: iceCanidate IS NULL" << error.description.c_str();
  }
  return iceCanidate;
}

} // namespace

namespace gloer {
namespace net {
namespace wrtc {

/*const boost::posix_time::time_duration WRTCSession::timerDeadlinePeriod =
    boost::posix_time::seconds(60);*/

WRTCSession::WRTCSession(NetworkManager* nm, const std::string& webrtcId, const std::string& wsId)
    : SessionBase(webrtcId), dataChannelstate_(webrtc::DataChannelInterface::kClosed), nm_(nm),
      wsId_(wsId) {}

WRTCSession::~WRTCSession() {
  LOG(INFO) << "~WRTCSession";
  CloseDataChannel(nm_, dataChannelI_, pci_);

  if (!onCloseCallback_) {
    LOG(WARNING) << "WRTCSession::onDataChannelMessage: Not set onMessageCallback_!";
    return;
  }

  onCloseCallback_(getId());

  if (nm_ && nm_->getWRTC().get()) {
    nm_->getWRTC()->unregisterSession(getId());
  }
}

/**
 * 16 Kbyte for the highest throughput, while also being the most portable one
 * @see https://viblast.com/blog/2015/2/5/webrtc-data-channel-message-size/
 **/
size_t WRTCSession::MAX_IN_MSG_SIZE_BYTE = 16 * 1024;
size_t WRTCSession::MAX_OUT_MSG_SIZE_BYTE = 16 * 1024;

void WRTCSession::createPeerConnectionObserver() {
  /*{
    if (!nm_->getWRTC()->workerThread_ || !nm_->getWRTC()->workerThread_.get()) {
      LOG(WARNING) << "WRTCSession::finishThreads: invalid workerThread_";
      return;
    }

    if (!nm_->getWRTC()->workerThread_->IsCurrent()) {
      return nm_->getWRTC()->workerThread_->Invoke<void>(
          RTC_FROM_HERE, [&] { return createPeerConnectionObserver(); });
    }
  }*/

  // nm_->getWRTC()->workerThread_->Post(RTC_FROM_HERE, this,
  // static_cast<uint32_t>(Message::RestartRelays)); nm_->getWRTC()->workerThread_->PostAt()

  /**
   * @brief WebRTC peer connection observer
   * Used to create WebRTC session paired with WebSocket session
   */
  std::shared_ptr<PCO> peerConnectionObserver =
      std::make_shared<PCO>(nm_, getId(), wsId_); // TODO: to private

  if (!peerConnectionObserver || !peerConnectionObserver.get()) {
    LOG(WARNING) << "empty peer_connection_observer";
    return;
  }

  // save at WRTCSession to prevent garbage collection
  peerConnectionObserver_ = peerConnectionObserver;
}

void WRTCSession::CloseDataChannel(
    NetworkManager* nm, rtc::scoped_refptr<webrtc::DataChannelInterface>& in_data_channel,
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> pci) {
  /*{
    if (!nm_->getWRTC()->workerThread_ || !nm_->getWRTC()->workerThread_.get()) {
      LOG(WARNING) << "WRTCSession::finishThreads: invalid workerThread_";
      return;
    }

    if (!nm_->getWRTC()->workerThread_->IsCurrent()) {
      return nm_->getWRTC()->workerThread_->Invoke<void>(
          RTC_FROM_HERE, [&] { return CloseDataChannel(nm, in_data_channel, pci); });
    }
  }*/

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
      if (in_data_channel->state() != webrtc::DataChannelInterface::kClosed) {
        in_data_channel->Close();
      }
      in_data_channel.release();
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

    if (pci) {
      if (pci->signaling_state() != webrtc::PeerConnectionInterface::kClosed) {
        pci->Close();
      }
      pci.release();
    }
  }
}

void WRTCSession::createPeerConnection() {
  /*{
    if (!nm_->getWRTC()->workerThread_ || !nm_->getWRTC()->workerThread_.get()) {
      LOG(WARNING) << "WRTCSession::finishThreads: invalid workerThread_";
      return;
    }

    if (!nm_->getWRTC()->workerThread_->IsCurrent()) {
      return nm_->getWRTC()->workerThread_->Invoke<void>(RTC_FROM_HERE,
                                                         [&] { return createPeerConnection(); });
    }
  }*/

  LOG(WARNING) << "creating portAllocator_...";

  // see https://github.com/sourcey/libsourcey/blob/master/src/webrtc/include/scy/webrtc/peer.h
  // see https://github.com/sourcey/libsourcey/blob/master/src/webrtc/src/peer.cpp
  std::unique_ptr<cricket::BasicPortAllocator> portAllocator_;

  if (!portAllocator_) {
    portAllocator_.reset(new cricket::BasicPortAllocator(nm_->getWRTC()->wrtcNetworkManager_.get(),
                                                         nm_->getWRTC()->socketFactory_.get()));
  }
  portAllocator_->SetPortRange(/* minPort */ 60000, /* maxPort */ 60001);

  if (!portAllocator_ || !portAllocator_.get()) {
    LOG(WARNING) << "WRTCServer::createPeerConnection: invalid portAllocator_";
    return;
  }
  LOG(WARNING) << "creating PeerConnection...";

  {
    rtc::CritScope lock(&nm_->getWRTC()->pcMutex_);
    // prevents pci_ garbage collection by 'operator='
    if (nm_->getWRTC()->peerConnectionFactory_.get() == nullptr) {
      LOG(WARNING) << "Error: Invalid CreatePeerConnectionFactory.";
      return;
    }

    pci_ = nm_->getWRTC()->peerConnectionFactory_->CreatePeerConnection(
        nm_->getWRTC()->getWRTCConf(), nullptr /*std::move(portAllocator_)*/,
        /* cert_generator */ nullptr, peerConnectionObserver_.get());
    if (!pci_ || !pci_.get()) {
      LOG(WARNING) << "WRTCServer::createPeerConnection: empty PCI";
      return;
    }
  }
}

void WRTCSession::setObservers() {
  /*{
    if (!nm_->getWRTC()->workerThread_ || !nm_->getWRTC()->workerThread_.get()) {
      LOG(WARNING) << "WRTCSession::createPeerConnection: invalid workerThread_";
      return;
    }

    if (!nm_->getWRTC()->workerThread_->IsCurrent()) {
      return nm_->getWRTC()->workerThread_->Invoke<void>(RTC_FROM_HERE,
                                                         [&] { return setObservers(); });
    }
  }*/

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

/*bool WRTCSession::isExpired() const {
  const bool isTimerExpired = boost::posix_time::second_clock::local_time() > timerDeadline;
  if (isTimerExpired) {
    return true;
  }
  return false;
}*/

/*
rtc::scoped_refptr<webrtc::PeerConnectionInterface> WRTCSession::getPCI() const {
  return pci_;
}*/

/*rtc::scoped_refptr<webrtc::DataChannelInterface> WRTCSession::getDataChannelI() const {
  return dataChannelI_;
}*/

void WRTCSession::send(const std::string& data) {
  /*LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::sendDataViaDataChannel const std::string&";*/
  WRTCSession::sendDataViaDataChannel(nm_, shared_from_this(), data);
}

bool WRTCSession::sendDataViaDataChannel(NetworkManager* nm, std::shared_ptr<WRTCSession> wrtcSess,
                                         const std::string& data) {

  /*{
    if (!nm->getWRTC()->workerThread_ || !nm->getWRTC()->workerThread_.get()) {
      LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: invalid workerThread_";
      return false;
    }

    if (!nm->getWRTC()->workerThread_->IsCurrent()) {
      return nm->getWRTC()->workerThread_->Invoke<bool>(
          RTC_FROM_HERE, [&] { return sendDataViaDataChannel(nm, wrtcSess, data); });
    }
  }*/

  /*LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::sendDataViaDataChannel std::shared_ptr<WRTCSession> wrtcSess, const "
               "std::string& data";*/

  if (!wrtcSess || !wrtcSess.get()) {
    LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: wrtc session is not established";
    return false;
  }

  if (!wrtcSess->isDataChannelOpen()) {
    LOG(WARNING) << "sendDataViaDataChannel: dataChannel not open!";
    return false;
  }

  // Construct a buffer and copy the specified number of bytes into it. The
  // source array may be (const) uint8_t*, int8_t*, or char*.
  webrtc::DataBuffer buffer(rtc::CopyOnWriteBuffer(data.c_str(), data.size()), /* binary */ false);
  sendDataViaDataChannel(nm, wrtcSess, std::move(buffer));
  return true;
}

/*class MessageHandler : rtc::MessageHandler {
public:
  MessageHandler() {}
  ~MessageHandler() { ; }
  void OnMessage(rtc::Message* msg){};
};*/

bool WRTCSession::sendDataViaDataChannel(NetworkManager* nm, std::shared_ptr<WRTCSession> wrtcSess,
                                         const webrtc::DataBuffer& buffer) {

  /*{
    if (!nm->getWRTC()->workerThread_ || !nm->getWRTC()->workerThread_.get()) {
      LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: invalid workerThread_";
      return false;
    }

    if (!nm->getWRTC()->workerThread_->IsCurrent()) {
      // Uses Send() internally, which blocks the current thread until execution
      // is complete.
      // NOTE: This function can only be called when synchronous calls are allowed.
      return nm->getWRTC()->workerThread_->Invoke<bool>(
          RTC_FROM_HERE, [&] { return sendDataViaDataChannel(nm, wrtcSess, buffer); });
    }
  }*/

  /*rtc::MessageHandler* mh = new MessageHandler();
  nm->getWRTC()->workerThread_->Post(RTC_FROM_HERE, mh);*/

  /*LOG(INFO) << std::this_thread::get_id() << ":"
              << "WRTCSession::sendDataViaDataChannel const webrtc::DataBuffer& buffer";*/

  if (!buffer.size()) {
    LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: Invalid messageBuffer";
    return false;
  }

  if (buffer.size() > MAX_OUT_MSG_SIZE_BYTE) {
    LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: Too big messageBuffer of size "
                 << buffer.size();
    return false;
  }

  if (!wrtcSess || !wrtcSess.get()) {
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

webrtc::SessionDescriptionInterface* WRTCSession::createSessionDescription(const std::string& type,
                                                                           const std::string& sdp) {
  /*{
    if (!nm_->getWRTC()->workerThread_ || !nm_->getWRTC()->workerThread_.get()) {
      LOG(WARNING) << "WRTCSession::createSessionDescription: invalid workerThread_";
      return nullptr;
    }

    if (!nm_->getWRTC()->workerThread_->IsCurrent()) {
      return nm_->getWRTC()->workerThread_->Invoke<webrtc::SessionDescriptionInterface*>(
          RTC_FROM_HERE, [&] { return createSessionDescription(sdp); });
    }
  }*/

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "createSessionDescriptionFromJson";

  webrtc::SdpParseError error;
  webrtc::SessionDescriptionInterface* sdi = webrtc::CreateSessionDescription(type, sdp, &error);
  if (sdi == nullptr) {
    LOG(WARNING) << "createSessionDescriptionFromJson:: SDI IS NULL" << error.description.c_str();
    LOG(WARNING) << error.description;
  }
  return sdi;
}

webrtc::DataChannelInterface::DataState WRTCSession::updateDataChannelState() {
  /*{
    if (!nm_->getWRTC()->workerThread_ || !nm_->getWRTC()->workerThread_.get()) {
      LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: invalid workerThread_";
      return webrtc::DataChannelInterface::kClosed;
    }

    if (!nm_->getWRTC()->workerThread_->IsCurrent()) {
      return nm_->getWRTC()->workerThread_->Invoke<webrtc::DataChannelInterface::DataState>(
          RTC_FROM_HERE, [&] { return updateDataChannelState(); });
    }
  }*/

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
  /*{
    if (!nm_->getWRTC()->workerThread_ || !nm_->getWRTC()->workerThread_.get()) {
      LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: invalid workerThread_";
      return;
    }

    if (!nm_->getWRTC()->workerThread_->IsCurrent()) {
      return nm_->getWRTC()->workerThread_->Invoke<void>(
          RTC_FROM_HERE, [&] { return createAndAddIceCandidate(message_object); });
    }
  }*/

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
    if (!pci_->AddIceCandidate(candidate_object.get())) {
      LOG(WARNING) << "createAndAddIceCandidate: Failed to apply the received candidate!";
      return;
    }
  }
}

bool WRTCSession::isDataChannelOpen() {
  /*LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::isDataChannelOpen";*/
  if (!dataChannelI_ || !dataChannelI_.get()) {
    LOG(WARNING) << "WRTCSession::isDataChannelOpen: Data channel is not established";
    return webrtc::DataChannelInterface::kClosed;
  }

  return dataChannelstate_ == webrtc::DataChannelInterface::kOpen;
}

void WRTCSession::createDCI() {
  /*{
    if (!nm_->getWRTC()->workerThread_ || !nm_->getWRTC()->workerThread_.get()) {
      LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: invalid workerThread_";
      return;
    }

    if (!nm_->getWRTC()->workerThread_->IsCurrent()) {
      return nm_->getWRTC()->workerThread_->Invoke<void>(RTC_FROM_HERE,
                                                         [&] { return createDCI(); });
    }
  }*/

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
}

void WRTCSession::SetRemoteDescription(
    webrtc::SessionDescriptionInterface* clientSessionDescription) {
  LOG(INFO) << "SetRemoteDescription...";

  /*{
    if (!nm_->getWRTC()->workerThread_ || !nm_->getWRTC()->workerThread_.get()) {
      LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: invalid workerThread_";
      return;
    }

    if (!nm_->getWRTC()->workerThread_->IsCurrent()) {
      return nm_->getWRTC()->workerThread_->Invoke<void>(
          RTC_FROM_HERE, [&] { return SetRemoteDescription(clientSessionDescription); });
    }
  }*/

  if (!clientSessionDescription) {
    LOG(WARNING) << "empty clientSessionDescription";
  }

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

  /*{
    if (!nm_->getWRTC()->workerThread_ || !nm_->getWRTC()->workerThread_.get()) {
      LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: invalid workerThread_";
      return;
    }

    if (!nm_->getWRTC()->workerThread_->IsCurrent()) {
      return nm_->getWRTC()->workerThread_->Invoke<void>(RTC_FROM_HERE,
                                                         [&] { return CreateAnswer(); });
    }
  }*/

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

  isFullyCreated_ = true; // TODO

  /*LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::OnDataChannelMessage";*/

  // lastRecievedMsgTime = boost::posix_time::second_clock::local_time();
  // timerDeadline = lastRecievedMsgTime + timerDeadlinePeriod;

  if (!buffer.size()) {
    LOG(WARNING) << "WRTCSession::onDataChannelMessage: Invalid messageBuffer";
    return;
  }

  if (buffer.size() > MAX_IN_MSG_SIZE_BYTE) {
    LOG(WARNING) << "WRTCSession::onDataChannelMessage: Too big messageBuffer of size "
                 << buffer.size();
    return;
  }

  const std::string data = std::string(buffer.data.data<char>(), buffer.size());

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

  // handleIncomingJSON(data);

  if (connectionChecker_ && connectionChecker_->onRemoteActivity(data)) {
    // got ping-pong messages to check connection status
    // return;
  }

  if (!onMessageCallback_) {
    LOG(WARNING) << "WRTCSession::onDataChannelMessage: Not set onMessageCallback_!";
    return;
  }

  onMessageCallback_(getId(), data);

  // send back?
  // WRTCSession::sendDataViaDataChannel(nm_, shared_from_this(), buffer);
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
  LOG(INFO) << "registered observer";

  wrtcSess->updateDataChannelState();

  {
    /**
     * offerers periodically check the connection and may reinitialise it.
     * The answerer side will recreate the peerconnection on receiving the offer
     * delay the reinit to not call the PeerConnectivityChecker destructor in its own callback
     **/
    const auto sid = wrtcSess->getId();
    const auto nm = wrtcSess->nm_;
    wrtcSess->connectionChecker_ = std::make_unique<PeerConnectivityChecker>(
        wrtcSess->dataChannelI_,
        /* ConnectivityLostCallback */ [nm, /* need weak ownership */ sid]() {
          // bool needStop = true;
          LOG(WARNING) << "WrtcServer::handleAllPlayerMessages: session timer expired";
          if (nm && /*wrtcSess && wrtcSess.get() &&*/ nm->getWRTC() && nm->getWRTC().get()) {
            // needStop = true;
            nm->getWRTC()->unregisterSession(sid);
            // nm->getWRTC()->CloseDataChannel(nm, wrtcSess->dataChannelI_, pci_);
          }
          return true; // stop periodic checks
        });
  }
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

  if (wsSess && wsSess.get() && wsSess->isOpen())
    wsSess->send(payload); // TODO: use Task queue
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
    wsSess->send(payload); // TODO: use Task queue
}

void WRTCSession::onDataChannelOpen() {
  LOG(INFO) << "WRTCSession::onDataChannelOpen";
  nm_->getWRTC()->addDataChannelCount(1);
}

void WRTCSession::onDataChannelClose() {
  LOG(INFO) << "WRTCSession::onDataChannelClose";
  nm_->getWRTC()->subDataChannelCount(1);
}

} // namespace wrtc
} // namespace net
} // namespace gloer

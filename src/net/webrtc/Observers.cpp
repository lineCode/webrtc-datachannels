#include "net/webrtc/Observers.hpp" // IWYU pragma: associated
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include "net/webrtc/WRTCServer.hpp"
#include "net/webrtc/WRTCSession.hpp"
#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <rtc_base/scoped_ref_ptr.h>
#include <string>
#include <thread>
#include <webrtc/api/peerconnectioninterface.h>

namespace utils {
namespace net {

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

void CSDO::OnSuccess(webrtc::SessionDescriptionInterface* sdi) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "CreateSessionDescriptionObserver::OnSuccess";
  if (sdi == nullptr) {
    LOG(WARNING) << "CSDO::OnSuccess INVALID SDI";
    return;
  }

  /*LOG(INFO) << std::this_thread::get_id() << ":"
            << "CreateSessionDescriptionObserver::OnSuccess";
  parent.onSuccessCSD(desc);*/

  if (!nm_->getWRTC()) {
    LOG(WARNING) << "empty m_observer";
    return;
  }

  auto spt = wrtcSess_.lock(); // Has to be copied into a shared_ptr before usage
  if (spt) {
    spt.get()->onAnswerCreated(sdi);
  } else {
    LOG(WARNING) << "wrtcSess_ expired";
    return;
  }
}

void CSDO::OnFailure(const std::string& error) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "CreateSessionDescriptionObserver::OnFailure\n"
            << error;
}

// Change in state of the Data Channel.
void DCO::OnStateChange() {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "DCO::OnStateChange";

  if (!nm_->getWRTC()) {
    LOG(WARNING) << "empty m_observer";
    return;
  }

  // TODO: need it? >>>
  // m_observer->data_channel_count++;
  auto spt = wrtcSess_.lock(); // Has to be copied into a shared_ptr before usage
  if (spt) {

    if (spt)
      spt->updateDataChannelState();

    webrtc::DataChannelInterface::DataState state = webrtc::DataChannelInterface::kClosed;

    if (spt && spt->dataChannelI_ && spt->dataChannelI_.get())
      state = spt->dataChannelI_->state();

    switch (state) {
    case webrtc::DataChannelInterface::kConnecting: {
      LOG(INFO) << "DCO::OnStateChange: data channel connecting!";
      break;
    }
    case webrtc::DataChannelInterface::kOpen: {
      if (spt)
        spt->onDataChannelOpen();
      LOG(INFO) << "DCO::OnStateChange: data channel open!";
      break;
    }
    case webrtc::DataChannelInterface::kClosing: {
      LOG(INFO) << "DCO::OnStateChange: data channel closing!";
      break;
    }
    case webrtc::DataChannelInterface::kClosed: {
      if (spt)
        spt->onDataChannelClose();
      LOG(INFO) << "DCO::OnStateChange: data channel not open!";
      break;
    }
    default: { LOG(INFO) << "DCO::OnStateChange: unknown data channel state! " << state; }
    }
  } else {
    LOG(WARNING) << "wrtcSess_ expired";
    return;
  }
}

void DCO::OnBufferedAmountChange(uint64_t /* previous_amount */) {}

// Message received.
void DCO::OnMessage(const webrtc::DataBuffer& buffer) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "DCO::OnMessage";

  if (!nm_->getWRTC()) {
    LOG(WARNING) << "empty m_observer";
    return;
  }

  auto spt = wrtcSess_.lock(); // Has to be copied into a shared_ptr before usage
  if (spt) {
    spt.get()->onDataChannelMessage(buffer);
  } else {
    LOG(WARNING) << "wrtcSess_ expired";
    return;
  }
}

// Triggered when a remote peer opens a data channel.
void PCO::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "DCO::OnDataChannel";
  if (!nm_->getWRTC()) {
    LOG(WARNING) << "empty m_observer";
    return;
  }

  if (!channel || !channel.get()) {
    LOG(WARNING) << "OnIceCandidate: empty DataChannelInterface";
    return;
  }

  /*auto spt = wrtcSess_.lock(); // Has to be copied into a shared_ptr before usage
  if (spt) {
    spt.get()->onDataChannelCreated(channel);
  } else {
    LOG(WARNING) << "wrtcSess_ expired";
  }*/

  std::shared_ptr<WRTCSession> wrtcSess = nm_->getWRTC()->getSessById(webrtcConnId_);
  if (wrtcSess == nullptr || !wrtcSess.get()) {
    LOG(WARNING) << "PCO::OnDataChannel: invalid webrtc session with id = " << webrtcConnId_;
    return;
  }

  WRTCSession::onDataChannelCreated(nm_, wrtcSess, channel);
}

// Override ICE candidate.
void PCO::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "DCO::OnIceCandidate";

  if (!candidate) {
    LOG(WARNING) << "OnIceCandidate: empty candidate";
    return;
  }

  if (!nm_->getWRTC()) {
    LOG(WARNING) << "OnIceCandidate: empty m_observer";
    return;
  }

  WRTCSession::onIceCandidate(nm_, wsConnId_, candidate);
}

void PCO::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "PeerConnectionObserver::SignalingChange(" << new_state << ")";
}

// Triggered when media is received on a new stream from remote peer.
void PCO::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> /* stream*/) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "PeerConnectionObserver::AddStream";
}

// Triggered when a remote peer close a stream.
void PCO::OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> /* stream*/) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "PeerConnectionObserver::RemoveStream";
}

// Triggered when renegotiation is needed. For example, an ICE restart
// has begun.
void PCO::OnRenegotiationNeeded() {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "PeerConnectionObserver::RenegotiationNeeded";
}

// Called any time the IceConnectionState changes.
//
// Note that our ICE states lag behind the standard slightly. The most
// notable differences include the fact that "failed" occurs after 15
// seconds, not 30, and this actually represents a combination ICE + DTLS
// state, so it may be "failed" if DTLS fails while ICE succeeds.
void PCO::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "PCO::IceConnectionChange(" << static_cast<int>(new_state) << ")";

  std::string state;
  bool needClose = false;

  switch (new_state) {
  case webrtc::PeerConnectionInterface::kIceConnectionNew: {
    state = "kIceConnectionNew";
    break;
  }
  case webrtc::PeerConnectionInterface::kIceConnectionChecking: {
    state = "kIceConnectionChecking";
    break;
  }
  case webrtc::PeerConnectionInterface::kIceConnectionConnected: {
    state = "kIceConnectionConnected";
    break;
  }
  case webrtc::PeerConnectionInterface::kIceConnectionCompleted: {
    state = "kIceConnectionCompleted";
    break;
  }
  case webrtc::PeerConnectionInterface::kIceConnectionFailed: {
    state = "kIceConnectionFailed";
    needClose = true;
    break;
  }
  case webrtc::PeerConnectionInterface::kIceConnectionDisconnected: {
    state = "kIceConnectionDisconnected";
    needClose = true;
    break;
  }
  case webrtc::PeerConnectionInterface::kIceConnectionClosed: {
    state = "kIceConnectionClosed";
    needClose = true;
    break;
  }
  case webrtc::PeerConnectionInterface::kIceConnectionMax:
    // TODO
    /* not in
     * https://developer.mozilla.org/en-US/docs/Web/API/RTCPeerConnection/iceConnectionState
     */
    {
      state = "kIceConnectionMax";
      needClose = true;
      break;
    }
  default:
    state = "unknown";
    break;
  }

  if (needClose) {
    nm_->getWRTC()->unregisterSession(webrtcConnId_);
  }

  LOG(INFO) << "OnIceConnectionChange to " << state;
}

// Called any time the IceGatheringState changes.
void PCO::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "PeerConnectionObserver::IceGatheringChange(" << new_state << ")";

  std::string state;

  switch (new_state) {
  case webrtc::PeerConnectionInterface::kIceGatheringNew: {
    state = "kIceGatheringNew";
    break;
  }
  case webrtc::PeerConnectionInterface::kIceGatheringGathering: {
    state = "kIceGatheringGathering";
    break;
  }
  case webrtc::PeerConnectionInterface::kIceGatheringComplete: {
    state = "kIceGatheringComplete";
    break;
  }
  default:
    state = "unknown";
    break;
  }

  LOG(INFO) << "OnIceGatheringChange to " << state;
}

} // namespace net
} // namespace utils

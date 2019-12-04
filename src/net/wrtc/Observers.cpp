#include "net/wrtc/Observers.hpp" // IWYU pragma: associated
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/wrtc/WRTCSession.hpp"
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <webrtc/api/peerconnectioninterface.h>
#include <webrtc/rtc_base/scoped_ref_ptr.h>

namespace gloer {
namespace net {
namespace wrtc {

void CSDO::OnSuccess(webrtc::SessionDescriptionInterface* sdi) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "CreateSessionDescriptionObserver::OnSuccess";

  LOG(WARNING) << "CSDO::OnSuccess as " << (isServer_ ? "server" : "offer");

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

  auto spt = wrtcSess_.lock();
  if (spt) {
    if (isServer_) {
      spt->updateDataChannelState();
      spt->onAnswerCreated(sdi);
      spt->updateDataChannelState();
    } else {
      spt->updateDataChannelState();
      spt->onOfferCreated(sdi);
      spt->updateDataChannelState();
    }
  } else {
    LOG(WARNING) << "wrtcSess_ expired";
    return;
  }
}

// PeerConnection cannot create an answer in a state other than have-remote-offer or
// have-local-pranswer
void CSDO::OnFailure(const std::string& error) {
  LOG(WARNING) << std::this_thread::get_id() << ":"
               << "CreateSessionDescriptionObserver::OnFailure\n"
               << error;
  LOG(WARNING) << "CSDO::OnFailure";

  if (!nm_->getWRTC()) {
    LOG(WARNING) << "empty m_observer";
    return;
  }

  auto spt = wrtcSess_.lock();
  if (spt) {
    spt->updateDataChannelState();
    spt->setFullyCreated(true); // allows auto-deletion
  } else {
    LOG(WARNING) << "wrtcSess_ expired";
    return;
  }
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

  auto spt = wrtcSess_.lock();
  if (spt) {
    auto wrtcSessId = spt->getId(); // remember id before session deletion
    spt->updateDataChannelState();

    webrtc::DataChannelInterface::DataState state = webrtc::DataChannelInterface::kClosed;

    if (spt && spt->dataChannelI_ && spt->dataChannelI_.get()) {
      state = spt->dataChannelI_->state();
    }

    switch (state) {
    case webrtc::DataChannelInterface::kConnecting: {
      if (spt) {
        RTC_DCHECK(spt->isDataChannelOpen() == false);
        // spt->onDataChannelConnecting();
      }
      LOG(INFO) << "DCO::OnStateChange: data channel connecting!";
      break;
    }
    case webrtc::DataChannelInterface::kOpen: {
      if (spt) {
        RTC_DCHECK(spt->isDataChannelOpen() == true);
        // spt->onDataChannelOpen();
      }

      ////////
      // spt->dataChannelI_->Send(webrtc::DataBuffer("1dsaadsadsasddddd"));

      LOG(INFO) << "DCO::OnStateChange: data channel open!";
      break;
    }
    case webrtc::DataChannelInterface::kClosing: {
      LOG(INFO) << "DCO::OnStateChange: data channel closing!";
      LOG(WARNING) << "webrtc::DataChannelInterface::kClosing";
      if (spt) {
        LOG(WARNING) << "webrtc::DataChannelInterface::kClosing";
        RTC_DCHECK(spt->isDataChannelOpen() == true);
        spt->setFullyCreated(true); // allows auto-deletion
      }
      break;
    }
    case webrtc::DataChannelInterface::kClosed: {
      LOG(WARNING) << "webrtc::DataChannelInterface::kClosed";
      if (spt) {
        RTC_DCHECK(spt->isDataChannelOpen() == false);
        // spt->onDataChannelClose();
      }

      {
        // unregisterSession don`t call CloseDataChannel -> onDataChannelClose
        // because dataChannelI_->state() != webrtc::DataChannelInterface::kClosed
        nm_->getWRTC_SM().unregisterSession(wrtcSessId);
      }

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

void DCO::OnBufferedAmountChange(uint64_t /* previous_amount */) {

  // TODO https://github.com/vmolsa/libcrtc/blob/master/src/rtcdatachannel.cc#L146
  /*if (_threshold && previous_amount > _threshold && _channel->buffered_amount() < _threshold) {
    onbufferedamountlow();
  }*/

  // TODO
  // https://github.com/RainwayApp/spitfire/blob/dbc65868d71b80f612acd3f4b8b9bd481c4a21df/Spitfire/DataChannelObserver.cpp

  /*
    if (!m_client)
        return;

    if (previousAmount <= m_channel->buffered_amount())
        return;

    callOnMainThread([protectedClient = makeRef(*m_client), amount = m_channel->buffered_amount()] {
        protectedClient->bufferedAmountIsDecreasing(static_cast<size_t>(amount));
    });
   */
}

// Message received.
void DCO::OnMessage(const webrtc::DataBuffer& buffer) {
  const std::string data = std::string(buffer.data.data<char>(), buffer.size());
  LOG(WARNING) << std::this_thread::get_id() << ":"
               << "DCO::OnMessage = " << data;

  if (!nm_->getWRTC()) {
    LOG(WARNING) << "empty m_observer";
    return;
  }

  // TODO if (buffer->binary)
  /*
    std::unique_ptr<webrtc::DataBuffer> protectedBuffer(new webrtc::DataBuffer(buffer));
    callOnMainThread([protectedClient = makeRef(*m_client), buffer = WTFMove(protectedBuffer)] {
        const char* data = reinterpret_cast<const char*>(buffer->data.data<char>());
        if (buffer->binary)
            protectedClient->didReceiveRawData(data, buffer->size());
        else
            protectedClient->didReceiveStringData(String::fromUTF8(data, buffer->size()));
    });
   */

  auto spt = wrtcSess_.lock();
  const auto wrtcSessId = spt->getId();
  if (spt) {
    if (spt->isClosing()) {
      // session is closing...
      nm_->getWRTC_SM().unregisterSession(wrtcSessId);
      return;
    }
    spt->onDataChannelMessage(buffer);
  } else {
    LOG(WARNING) << "wrtcSess_ expired";
    return;
  }
}

// Triggered when a remote peer opens a data channel.
void PCO::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "PCO::OnDataChannel";

  RTC_DCHECK(nm_->getWRTC() != nullptr);
  if (!nm_->getWRTC()) {
    LOG(WARNING) << "empty m_observer";
    return;
  }

  RTC_DCHECK(channel.get() != nullptr);
  if (!channel || !channel.get()) {
    LOG(WARNING) << "OnDataChannel: empty DataChannelInterface";
    return;
  }

  /*auto spt = wrtcSess_.lock();
  if (spt) {
    spt->onDataChannelCreated(channel);
  } else {
    LOG(WARNING) << "wrtcSess_ expired";
  }*/

  LOG(WARNING) << "PCO::OnDataChannel for id = " << webrtcConnId_;
  std::shared_ptr<WRTCSession> wrtcSess = nm_->getWRTC_SM().getSessById(webrtcConnId_);

  RTC_DCHECK(wrtcSess.get() != nullptr);
  if (wrtcSess == nullptr || !wrtcSess.get()) {
    // LOG(WARNING) << "PCO::OnDataChannel: invalid webrtc session with id = " << webrtcConnId_;
    nm_->getWRTC_SM().unregisterSession(webrtcConnId_);
    return;
  }

  RTC_DCHECK(wrtcSess.get() != nullptr);
  if (wrtcSess.get()) {
    // calls wrtcSess->updateDataChannelState();
    wrtcSess->onDataChannelCreated(nm_, channel);
  }

  // TODO _manager->onIceCandidate(candidate->sdp_mid().c_str(), candidate->sdp_mline_index(),
  // sdp.c_str());
  // https://github.com/RainwayApp/spitfire/blob/master/Spitfire/PeerConnectionObserver.cpp
}

// Override ICE candidate.
void PCO::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "PCO::OnIceCandidate";

  RTC_DCHECK(candidate != nullptr);
  if (!candidate) {
    LOG(WARNING) << "OnIceCandidate: empty candidate";
    return;
  }

  // TODO filter candidate->candidate().url()

  RTC_DCHECK(nm_->getWRTC() != nullptr);
  if (!nm_->getWRTC()) {
    LOG(WARNING) << "OnIceCandidate: empty m_observer";
    return;
  }

  std::shared_ptr<WRTCSession> wrtcSess = nm_->getWRTC_SM().getSessById(webrtcConnId_);
  RTC_DCHECK(wrtcSess.get() != nullptr);
  if (!wrtcSess.get()) {
    LOG(WARNING) << "PCO::OnIceCandidate: invalid webrtc session with id = " << webrtcConnId_;
    nm_->getWRTC_SM().unregisterSession(webrtcConnId_);
    return;
  }

  RTC_DCHECK(wrtcSess.get() != nullptr);
  if (wrtcSess.get()) {
    wrtcSess->updateDataChannelState();
  }

  WRTCSession::onIceCandidate(nm_, wsConnId_, candidate);
}

void PCO::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "PeerConnectionObserver::SignalingChange(" << new_state << ")";

  std::string state;
  bool needClose = false;

  switch (new_state) {
    // @see w3c.github.io/webrtc-pc/#state-definitions
  case webrtc::PeerConnectionInterface::SignalingState::kClosed: {
    state = "kClosed";
    needClose = true;
    break;
  }
  case webrtc::PeerConnectionInterface::SignalingState::kStable: {
    /*
     * There is no offer/answer exchange in progress.
     * This is also the initial state,
     * in which case the local and remote descriptions are empty.
     */
    state = "kStable";
    break;
  }
  case webrtc::PeerConnectionInterface::SignalingState::kHaveLocalOffer: {
    state = "kHaveLocalOffer";
    break;
  }
  case webrtc::PeerConnectionInterface::SignalingState::kHaveRemoteOffer: {
    state = "kHaveRemoteOffer";
    break;
  }
  case webrtc::PeerConnectionInterface::SignalingState::kHaveLocalPrAnswer: {
    /*
     * A remote description of type "offer" has been successfully applied
     * and a local description of type "pranswer" has been successfully applied.
     */
    state = "kHaveLocalPrAnswer";
    break;
  }
  case webrtc::PeerConnectionInterface::SignalingState::kHaveRemotePrAnswer: {
    state = "kHaveRemotePrAnswer";
    break;
  }
  default:
    state = "unknown";
    break;
  }

  LOG(INFO) << "OnSignalingChange to " << state;

  {
    std::shared_ptr<WRTCSession> wrtcSess = nm_->getWRTC_SM().getSessById(webrtcConnId_);
    if (wrtcSess && wrtcSess.get()) {
      // LOG(WARNING) << "PCO::OnSignalingChange: invalid webrtc session with id = " <<
      // webrtcConnId_; nm_->getWRTC()->unregisterSession(webrtcConnId_); // use needClose

      if (needClose) {
        // LOG(WARNING) << "PCO::OnSignalingChange: closed session with id = " << webrtcConnId_;
        // wrtcSess->close_s(false); // called from unregisterSession
      }

      wrtcSess->updateDataChannelState();
    }
  }

  if (needClose) {
    nm_->getWRTC_SM().unregisterSession(webrtcConnId_);
  }
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

  std::shared_ptr<WRTCSession> wrtcSess = nm_->getWRTC_SM().getSessById(webrtcConnId_);
  if (wrtcSess.get()) {
    // LOG(WARNING) << "PCO::OnRemoveStream: invalid webrtc session with id = " <<
    // webrtcConnId_;
    // wrtcSess->close_s(false); // called from unregisterSession
  }
  nm_->getWRTC_SM().unregisterSession(webrtcConnId_);
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
    // @see w3c.github.io/webrtc-pc/#rtcpeerconnectionstate-enum
  case webrtc::PeerConnectionInterface::kIceConnectionNew: {
    // Waiting for the other to answer
    state = "kIceConnectionNew";
    break;
  }
  case webrtc::PeerConnectionInterface::kIceConnectionChecking: {
    // Trying to establish a connection with the other
    state = "kIceConnectionChecking";
    break;
  }
  case webrtc::PeerConnectionInterface::kIceConnectionConnected: {
    // 	Established a connection and started a chat
    state = "kIceConnectionConnected";
    break;
  }
  case webrtc::PeerConnectionInterface::kIceConnectionCompleted: {
    // In chat
    state = "kIceConnectionCompleted";
    break;
  }
  case webrtc::PeerConnectionInterface::kIceConnectionFailed: {
    LOG(WARNING) << "PeerConnectionInterface::kIceConnectionFailed";
    state = "kIceConnectionFailed";
    needClose = true;
    break;
  }
  case webrtc::PeerConnectionInterface::kIceConnectionDisconnected: {
    LOG(WARNING) << "PeerConnectionInterface::kIceConnectionDisconnected";
    state = "kIceConnectionDisconnected";
    needClose = true;
    break;
  }
  case webrtc::PeerConnectionInterface::kIceConnectionClosed: {
    LOG(WARNING) << "PeerConnectionInterface::kIceConnectionClosed";
    state = "kIceConnectionClosed";
    needClose = true;
    break;
  }
  case webrtc::PeerConnectionInterface::kIceConnectionMax:
    // TODO
    /* not in
     * developer.mozilla.org/en-US/docs/Web/API/RTCPeerConnection/iceConnectionState
     */
    {
      LOG(WARNING) << "PeerConnectionInterface::kIceConnectionMax";
      state = "kIceConnectionMax";
      needClose = true;
      break;
    }
  default:
    state = "unknown";
    break;
  }

  LOG(INFO) << "OnIceConnectionChange to " << state;

  {
    std::shared_ptr<WRTCSession> wrtcSess = nm_->getWRTC_SM().getSessById(webrtcConnId_);
    if (wrtcSess.get()) {
      // LOG(WARNING) << "PCO::OnIceConnectionChange: invalid webrtc session with id = "
      //             << webrtcConnId_;
      // nm_->getWRTC()->unregisterSession(webrtcConnId_); // use needClose

      if (needClose) {
        // LOG(WARNING) << "PCO::OnIceConnectionChange: closed session with id = " << webrtcConnId_;
        // wrtcSess->close_s(false); // called from unregisterSession
      }

      wrtcSess->updateDataChannelState();
    }
  }

  if (needClose) {
    nm_->getWRTC_SM().unregisterSession(webrtcConnId_);
  }
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

void SSDO::OnSuccess() {

  /*
   * TODO
  2019/01/30 22:08:24 311001      INFO [Observers.cpp->OnFailure:46]
  139998888195840:CreateSessionDescriptionObserver::OnFailure PeerConnection cannot create an
  answer in a state other than have-remote-offer or have-local-pranswer
  */
  /*auto spt = wrtcSess_.lock();
  if (spt) {
    spt->CreateAnswer();
  } else {
    LOG(WARNING) << "wrtcSess_ expired";
    return;
  }*/
}

void SSDO::OnFailure(const std::string& error) {
  LOG(WARNING) << "SSDO::OnFailure: Failure to set a sesion description. " << error;
  auto spt = wrtcSess_.lock();
  LOG(WARNING) << "SSDO::OnFailure";
  if (spt) {
    spt->setFullyCreated(true); // allows auto-deletion
  } else {
    LOG(WARNING) << "wrtcSess_ expired";
    return;
  }
}

} // namespace wrtc
} // namespace net
} // namespace gloer

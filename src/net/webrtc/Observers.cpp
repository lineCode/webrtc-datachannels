#include "net/webrtc/Observers.hpp" // IWYU pragma: associated
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include "net/webrtc/WRTCServer.hpp"
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

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

void CSDO::OnSuccess(webrtc::SessionDescriptionInterface* sdi) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "CreateSessionDescriptionObserver::OnSuccess";
  if (sdi == nullptr) {
    LOG(INFO) << "CSDO::OnSuccess INVALID SDI";
  }
  /*LOG(INFO) << std::this_thread::get_id() << ":"
            << "CreateSessionDescriptionObserver::OnSuccess";
  parent.onSuccessCSD(desc);*/
  if (!wrtcServer_) {
    LOG(INFO) << "empty m_observer";
  }
  wrtcServer_->OnAnswerCreated(sdi);
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
  if (!wrtcServer_) {
    LOG(INFO) << "empty m_observer";
  }
  // m_observer->data_channel_count++;
  /*if (!m_observer->m_WRTC->data_channel) {
    LOG(INFO) << "DCO::OnStateChange: data channel empty!";
    return;
  }

  m_observer->m_WRTC->updateDataChannelState();
  webrtc::DataChannelInterface::DataState state =
  m_observer->m_WRTC->data_channel->state();

  switch (state) {
    case webrtc::DataChannelInterface::kConnecting:
      {
        LOG(INFO) << "DCO::OnStateChange: data channel connecting!"; break;
      }
    case webrtc::DataChannelInterface::kOpen:
      {
        m_observer->onDataChannelOpen();
        LOG(INFO) << "DCO::OnStateChange: data channel open!";
        break;
      }
    case webrtc::DataChannelInterface::kClosing:
      {
        LOG(INFO) << "DCO::OnStateChange: data channel closing!";
        break;
      }
    case webrtc::DataChannelInterface::kClosed:
      {
        m_observer->onDataChannelClose();
        LOG(INFO) << "DCO::OnStateChange: data channel not open!";
        break;
      }
    default:
      {
        LOG(INFO) << "DCO::OnStateChange: unknown data channel state! " <<
  state;
      }
  }*/
}

void DCO::OnBufferedAmountChange(uint64_t /* previous_amount */) {}

// Message received.
void DCO::OnMessage(const webrtc::DataBuffer& buffer) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "DCO::OnMessage";
  if (!wrtcServer_) {
    LOG(INFO) << "empty m_observer";
  }
  wrtcServer_->OnDataChannelMessage(buffer);
}

// Triggered when a remote peer opens a data channel.
void PCO::OnDataChannel(
    rtc::scoped_refptr<webrtc::DataChannelInterface> channel) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "DCO::OnDataChannel";
  if (!wrtcServer_) {
    LOG(INFO) << "empty m_observer";
  }
  wrtcServer_->OnDataChannelCreated(channel);
}

// Override ICE candidate.
void PCO::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "DCO::OnIceCandidate";
  if (!wrtcServer_) {
    LOG(INFO) << "empty m_observer";
  }
  wrtcServer_->OnIceCandidate(candidate);
}

void PCO::OnSignalingChange(
    webrtc::PeerConnectionInterface::SignalingState new_state) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "PeerConnectionObserver::SignalingChange(" << new_state << ")";
}

// Triggered when media is received on a new stream from remote peer.
void PCO::OnAddStream(
    rtc::scoped_refptr<webrtc::MediaStreamInterface> /* stream*/) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "PeerConnectionObserver::AddStream";
}

// Triggered when a remote peer close a stream.
void PCO::OnRemoveStream(
    rtc::scoped_refptr<webrtc::MediaStreamInterface> /* stream*/) {
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
void PCO::OnIceConnectionChange(
    webrtc::PeerConnectionInterface::IceConnectionState new_state) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "PCO::IceConnectionChange(" << static_cast<int>(new_state)
            << ")";
  switch (new_state) {
  case webrtc::PeerConnectionInterface::kIceConnectionNew: {
    break;
  }
  case webrtc::PeerConnectionInterface::kIceConnectionChecking: {
    break;
  }
  case webrtc::PeerConnectionInterface::kIceConnectionConnected:
    break;
  case webrtc::PeerConnectionInterface::kIceConnectionCompleted: {
    break;
  }
  case webrtc::PeerConnectionInterface::kIceConnectionFailed: {
    break;
  }
  case webrtc::PeerConnectionInterface::kIceConnectionDisconnected: {
    break;
  }
  case webrtc::PeerConnectionInterface::kIceConnectionClosed: {
    break;
  }
  case webrtc::PeerConnectionInterface::kIceConnectionMax:
    // TODO
    /* not in
     * https://developer.mozilla.org/en-US/docs/Web/API/RTCPeerConnection/iceConnectionState
     */
    { break; }
  }
}

// Called any time the IceGatheringState changes.
void PCO::OnIceGatheringChange(
    webrtc::PeerConnectionInterface::IceGatheringState new_state) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "PeerConnectionObserver::IceGatheringChange(" << new_state
            << ")";
}

} // namespace net
} // namespace utils

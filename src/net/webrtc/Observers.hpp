#pragma once

#include <api/datachannelinterface.h>
#include <api/jsep.h>
#include <cstdint>
#include <rtc_base/refcount.h>
#include <string>
#include <webrtc/api/peerconnectioninterface.h>
#include <webrtc/base/macros.h>
#include <webrtc/rtc_base/scoped_ref_ptr.h>

namespace webrtc {
class MediaStreamInterface;
} // namespace webrtc

namespace utils {
namespace net {

class WRTCServer;
// class NetworkManager; // TODO

// PeerConnection events.
// see
// https://github.com/sourcey/libsourcey/blob/ce311ff22ca02c8a83df7162a70f6aa4f760a761/doc/api-webrtc.md
class PCO : public webrtc::PeerConnectionObserver {
public:
  // Constructor taking a few callbacks.
  PCO(WRTCServer& observer) : wrtcServer_(&observer) {}

  // Triggered when a remote peer opens a data channel.
  void OnDataChannel(
      rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override;

  // Override ICE candidate.
  void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;

  void OnSignalingChange(
      webrtc::PeerConnectionInterface::SignalingState new_state) override;

  // Triggered when media is received on a new stream from remote peer.
  void OnAddStream(
      rtc::scoped_refptr<webrtc::MediaStreamInterface> /* stream*/) override;

  // Triggered when a remote peer close a stream.
  void OnRemoveStream(
      rtc::scoped_refptr<webrtc::MediaStreamInterface> /* stream*/) override;

  // Triggered when renegotiation is needed. For example, an ICE restart
  // has begun.
  void OnRenegotiationNeeded() override;

  // Called any time the IceConnectionState changes.
  //
  // Note that our ICE states lag behind the standard slightly. The most
  // notable differences include the fact that "failed" occurs after 15
  // seconds, not 30, and this actually represents a combination ICE + DTLS
  // state, so it may be "failed" if DTLS fails while ICE succeeds.
  void OnIceConnectionChange(
      webrtc::PeerConnectionInterface::IceConnectionState new_state) override;

  // Called any time the IceGatheringState changes.
  void OnIceGatheringChange(
      webrtc::PeerConnectionInterface::IceGatheringState new_state) override;

  // TODO OnInterestingUsage

private:
  WRTCServer* wrtcServer_;

  // see
  // https://cs.chromium.org/chromium/src/remoting/protocol/webrtc_transport.cc?q=SetSessionDescriptionObserver&dr=CSs&l=148
  DISALLOW_COPY_AND_ASSIGN(PCO);
};

// DataChannel events.
class DCO : public webrtc::DataChannelObserver {
public:
  // Constructor taking a callback.
  DCO(WRTCServer& observer) : wrtcServer_(&observer) {}

  // Buffered amount change.
  void OnBufferedAmountChange(uint64_t /* previous_amount */) override;

  void OnStateChange() override;

  // Message received.
  void OnMessage(const webrtc::DataBuffer& buffer) override;

private:
  WRTCServer* wrtcServer_;

  // see
  // https://cs.chromium.org/chromium/src/remoting/protocol/webrtc_transport.cc?q=SetSessionDescriptionObserver&dr=CSs&l=148
  DISALLOW_COPY_AND_ASSIGN(DCO);
};

// Create SessionDescription events.
class CSDO : public webrtc::CreateSessionDescriptionObserver {
public:
  // Constructor taking a callback.
  CSDO(WRTCServer& observer) : wrtcServer_(&observer) {}

  /*void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;

  void OnFailure(const std::string& error) override;*/

  // Successfully created a session description.
  /*void OnSuccess(webrtc::SessionDescriptionInterface* desc) {
    OnAnswerCreated(desc);
  }*/

  // Failure to create a session description.
  // void OnFailure(const std::string& /* error */) {}

  void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;

  void OnFailure(const std::string& error) override;

  // SEE
  // https://github.com/sourcey/libsourcey/blob/master/src/webrtc/include/scy/webrtc/peer.h#L102
  // Unimplemented virtual function.
  void AddRef() const override { return; }

  // Unimplemented virtual function.
  rtc::RefCountReleaseStatus Release() const override {
    return rtc::RefCountReleaseStatus::kDroppedLastRef;
  }

private:
  WRTCServer* wrtcServer_;

  // see
  // https://cs.chromium.org/chromium/src/remoting/protocol/webrtc_transport.cc?q=SetSessionDescriptionObserver&dr=CSs&l=148
  DISALLOW_COPY_AND_ASSIGN(CSDO);
};

// Set SessionDescription events.
class SSDO : public webrtc::SetSessionDescriptionObserver {
public:
  // Default constructor.
  SSDO() {}

  // Successfully set a session description.
  void OnSuccess() override {}

  // Failure to set a sesion description.
  void OnFailure(const std::string& /* error */) override {}

  // SEE
  // https://github.com/sourcey/libsourcey/blob/master/src/webrtc/include/scy/webrtc/peer.h#L102
  // Unimplemented virtual function.
  void AddRef() const override { return; }

  // Unimplemented virtual function.
  rtc::RefCountReleaseStatus Release() const override {
    return rtc::RefCountReleaseStatus::kDroppedLastRef;
  }

  // see
  // https://cs.chromium.org/chromium/src/remoting/protocol/webrtc_transport.cc?q=SetSessionDescriptionObserver&dr=CSs&l=148
  DISALLOW_COPY_AND_ASSIGN(SSDO);
};

} // namespace net
} // namespace utils
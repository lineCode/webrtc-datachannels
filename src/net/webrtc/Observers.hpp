#pragma once

//#define BOOST_NO_EXCEPTIONS
//#define BOOST_NO_RTTI

#include "algorithm/DispatchQueue.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <string>
#include <thread>
#include <unordered_map>
#include <webrtc/api/peerconnectioninterface.h>
#include <webrtc/base/macros.h>
#include <webrtc/media/engine/webrtcmediaengine.h>
#include <webrtc/p2p/base/basicpacketsocketfactory.h>
#include <webrtc/p2p/client/basicportallocator.h>
#include <webrtc/pc/peerconnection.h>
#include <webrtc/pc/peerconnectionfactory.h>
#include <webrtc/rtc_base/physicalsocketserver.h>
#include <webrtc/rtc_base/scoped_ref_ptr.h>
#include <webrtc/rtc_base/ssladapter.h>
#include <webrtc/rtc_base/thread.h>

namespace utils {
namespace net {

class WRTCServer;
class NetworkManager;

// PeerConnection events.
// see
// https://github.com/sourcey/libsourcey/blob/ce311ff22ca02c8a83df7162a70f6aa4f760a761/doc/api-webrtc.md
class PCO : public webrtc::PeerConnectionObserver {
public:
  // Constructor taking a few callbacks.
  PCO(WRTCServer& observer) : m_observer(&observer) {}

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
  WRTCServer* m_observer;

  // see
  // https://cs.chromium.org/chromium/src/remoting/protocol/webrtc_transport.cc?q=SetSessionDescriptionObserver&dr=CSs&l=148
  DISALLOW_COPY_AND_ASSIGN(PCO);
};

// DataChannel events.
class DCO : public webrtc::DataChannelObserver {
public:
  // Constructor taking a callback.
  DCO(WRTCServer& observer) : m_observer(&observer) {}

  // Buffered amount change.
  void OnBufferedAmountChange(uint64_t /* previous_amount */) override;

  void OnStateChange() override;

  // Message received.
  void OnMessage(const webrtc::DataBuffer& buffer) override;

private:
  WRTCServer* m_observer;

  // see
  // https://cs.chromium.org/chromium/src/remoting/protocol/webrtc_transport.cc?q=SetSessionDescriptionObserver&dr=CSs&l=148
  DISALLOW_COPY_AND_ASSIGN(DCO);
};

// Create SessionDescription events.
class CSDO : public webrtc::CreateSessionDescriptionObserver {
public:
  // Constructor taking a callback.
  CSDO(WRTCServer& observer) : m_observer(&observer) {}

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
  WRTCServer* m_observer;

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

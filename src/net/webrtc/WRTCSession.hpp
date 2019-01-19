#pragma once

#include <api/datachannelinterface.h>
#include <cstdint>
#include <rapidjson/document.h>
#include <rtc_base/criticalsection.h>
#include <string>
#include <vector>
#include <webrtc/api/peerconnectioninterface.h>
#include <webrtc/rtc_base/scoped_ref_ptr.h>

namespace rtc {
class Thread;
} // namespace rtc

namespace webrtc {
class IceCandidateInterface;
} // namespace webrtc

namespace webrtc {
class SessionDescriptionInterface;
} // namespace webrtc

namespace utils {
namespace algo {
class DispatchQueue;
} // namespace algo
} // namespace utils

namespace utils {
namespace net {

class NetworkManager;
class DCO;
class PCO;
class SSDO;
class CSDO;
class WsSession;

class WRTCSession : public std::enable_shared_from_this<WRTCSession> {
public:
  // WRTCSession() {}

  WRTCSession(NetworkManager* nm, const std::string& webrtcId, const std::string& wsId,
              rtc::scoped_refptr<webrtc::PeerConnectionInterface> pci);

  void setObservers();

  // rtc::scoped_refptr<webrtc::PeerConnectionInterface> getPCI() const;

  // rtc::scoped_refptr<webrtc::DataChannelInterface> getDataChannelI() const;

  void setLocalDescription(webrtc::SessionDescriptionInterface* sdi);

  webrtc::DataChannelInterface::DataState updateDataChannelState();

  static void setRemoteDescriptionAndCreateAnswer(WsSession* clientWsSession, NetworkManager* nm,
                                                  const rapidjson::Document& message_object);

  void createAndAddIceCandidate(const rapidjson::Document& message_object);

  static void onDataChannelCreated(NetworkManager* nm, std::shared_ptr<WRTCSession> wrtcSess,
                                   rtc::scoped_refptr<webrtc::DataChannelInterface> channel);

  static void onIceCandidate(NetworkManager* nm, const webrtc::IceCandidateInterface* candidate);

  void onAnswerCreated(webrtc::SessionDescriptionInterface* desc);

  bool isDataChannelOpen();

  void onDataChannelMessage(const webrtc::DataBuffer& buffer);

  void onDataChannelOpen();

  void onDataChannelClose();

  static bool sendDataViaDataChannel(NetworkManager* nm, std::shared_ptr<WRTCSession> wrtcSess,
                                     const std::string& data);

  static bool sendDataViaDataChannel(NetworkManager* nm, std::shared_ptr<WRTCSession> wrtcSess,
                                     const webrtc::DataBuffer& buffer);

private:
  void createDCI();

  void SetRemoteDescription(webrtc::SessionDescriptionInterface* clientSessionDescription);

  void CreateAnswer(NetworkManager* nm);

private:
  NetworkManager* nm_;

  const std::string webrtcId_;

  // websocket session ID used to create WRTCSession
  // NOTE: websocket session may be deleted before/after webRTC session
  const std::string wsId_;

  // The observer that responds to session description set events. We don't
  // really use this one here. webrtc::SetSessionDescriptionObserver for
  // acknowledging and storing an offer or answer.
  std::unique_ptr<SSDO> localDescriptionObserver_;

  std::unique_ptr<SSDO> remoteDescriptionObserver_;

  // The observer that responds to data channel events.
  // webrtc::DataChannelObserver for data channel events like receiving SCTP
  // messages.
  std::unique_ptr<DCO> dataChannelObserver_; //(webRtcObserver);
                                             // rtc::scoped_refptr<PCO> peer_connection_observer
                                             // = new
                                             // rtc::RefCountedObject<PCO>(OnDataChannelCreated,
                                             // OnIceCandidate);

  // The observer that responds to session description creation events.
  // webrtc::CreateSessionDescriptionObserver for creating an offer or answer.
  std::unique_ptr<CSDO> createSDO_;
};

} // namespace net
} // namespace utils
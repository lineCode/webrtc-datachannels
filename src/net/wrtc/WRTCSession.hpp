#pragma once

#include "net/SessionBase.hpp"
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

namespace gloer {
namespace algo {
class DispatchQueue;
} // namespace algo
} // namespace gloer

namespace gloer {
namespace net {

class NetworkManager;

namespace ws {
class WsSession;
}

} // namespace net
} // namespace gloer

namespace gloer {
namespace net {
namespace wrtc {

class DCO;
class PCO;
class SSDO;
class CSDO;

class WRTCSession : public SessionBase, public std::enable_shared_from_this<WRTCSession> {
public:
  // WRTCSession() {} // TODO

  explicit WRTCSession(NetworkManager* nm, const std::string& webrtcId, const std::string& wsId);

  ~WRTCSession();

  void CloseDataChannel(NetworkManager* nm,
                        rtc::scoped_refptr<webrtc::DataChannelInterface>& in_data_channel,
                        rtc::scoped_refptr<webrtc::PeerConnectionInterface> pci_);

  bool handleIncomingJSON(std::shared_ptr<std::string> message) override;

  void send(std::shared_ptr<std::string> ss) override;

  void send(const std::string& ss) override;

  void setObservers();

  bool isExpired() const override;

  // rtc::scoped_refptr<webrtc::PeerConnectionInterface> getPCI() const;

  // rtc::scoped_refptr<webrtc::DataChannelInterface> getDataChannelI() const;

  void setLocalDescription(webrtc::SessionDescriptionInterface* sdi);

  webrtc::DataChannelInterface::DataState updateDataChannelState();

  void createAndAddIceCandidate(const rapidjson::Document& message_object);

  static void onDataChannelCreated(NetworkManager* nm, std::shared_ptr<WRTCSession> wrtcSess,
                                   rtc::scoped_refptr<webrtc::DataChannelInterface> channel);

  static void onIceCandidate(NetworkManager* nm, const std::string& wsConnId,
                             const webrtc::IceCandidateInterface* candidate);

  void onAnswerCreated(webrtc::SessionDescriptionInterface* desc);

  bool isOpen();

  void onDataChannelMessage(const webrtc::DataBuffer& buffer);

  void onDataChannelOpen();

  void onDataChannelClose();

  static bool sendDataViaDataChannel(NetworkManager* nm, std::shared_ptr<WRTCSession> wrtcSess,
                                     const std::string& data);

  static bool sendDataViaDataChannel(NetworkManager* nm, std::shared_ptr<WRTCSession> wrtcSess,
                                     const webrtc::DataBuffer& buffer);

  // TODO private >>

  rtc::scoped_refptr<webrtc::PeerConnectionInterface> pci_; // TODO: private

  // The data channel used to communicate.
  rtc::scoped_refptr<webrtc::DataChannelInterface> dataChannelI_;

  // The socket that the signaling thread and worker thread communicate on.
  // CustomSocketServer socket_server;
  // rtc::PhysicalSocketServer socket_server;
  // last updated DataChannel state
  webrtc::DataChannelInterface::DataState dataChannelstate_;

  // The observer that responds to session description set events. We don't
  // really use this one here. webrtc::SetSessionDescriptionObserver for
  // acknowledging and storing an offer or answer.
  rtc::scoped_refptr<SSDO> localDescriptionObserver_;

  rtc::scoped_refptr<SSDO> remoteDescriptionObserver_;

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
  rtc::scoped_refptr<CSDO> createSDO_;

  std::shared_ptr<PCO> peerConnectionObserver_;

  void createDCI();

  void SetRemoteDescription(webrtc::SessionDescriptionInterface* clientSessionDescription);

  void CreateAnswer();

  bool fullyCreated() const { return isFullyCreated_; }

  boost::posix_time::ptime lastRecievedMsgTime{boost::posix_time::second_clock::local_time()};

  static const boost::posix_time::time_duration timerDeadlinePeriod;

  boost::posix_time::ptime timerDeadline = lastRecievedMsgTime + timerDeadlinePeriod;

private:
  // rtc::CriticalSection peerConIMutex_; // TODO: to private

  bool isFullyCreated_{false}; // TODO

  /**
   * 16 Kbyte for the highest throughput, while also being the most portable one
   * @see https://viblast.com/blog/2015/2/5/webrtc-data-channel-message-size/
   **/
  static size_t MAX_IN_MSG_SIZE_BYTE;
  static size_t MAX_OUT_MSG_SIZE_BYTE;

  NetworkManager* nm_;

  // websocket session ID used to create WRTCSession
  // NOTE: websocket session may be deleted before/after webRTC session
  const std::string wsId_;
};

} // namespace wrtc
} // namespace net
} // namespace gloer

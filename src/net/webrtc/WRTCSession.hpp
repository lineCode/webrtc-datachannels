#pragma once

#include "net/SessionI.hpp"
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

class WRTCSession : public SessionI, public std::enable_shared_from_this<WRTCSession> {
public:
  // WRTCSession() {} // TODO

  explicit WRTCSession(NetworkManager* nm, const std::string& webrtcId, const std::string& wsId);

  ~WRTCSession();

  static void CloseDataChannel(NetworkManager* nm,
                               rtc::scoped_refptr<webrtc::DataChannelInterface>& in_data_channel,
                               rtc::scoped_refptr<webrtc::PeerConnectionInterface> pci_);

  ///

  std::shared_ptr<algo::DispatchQueue> getReceivedMessages() const;

  bool hasReceivedMessages() const;

  bool handleIncomingJSON(const webrtc::DataBuffer& buffer);

  std::string getId() const { return id_; }

  void send(std::shared_ptr<std::string> ss) override;

  void send(const std::string& ss) override;

  ///

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

  static void onIceCandidate(NetworkManager* nm, const std::string& wsConnId,
                             const webrtc::IceCandidateInterface* candidate);

  void onAnswerCreated(webrtc::SessionDescriptionInterface* desc);

  bool isDataChannelOpen();

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

private:
  void createDCI();

  void SetRemoteDescription(webrtc::SessionDescriptionInterface* clientSessionDescription);

  void CreateAnswer();

private:
  NetworkManager* nm_;

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

  std::shared_ptr<algo::DispatchQueue> receivedMessagesQueue_;
  const std::string id_;
};

} // namespace net
} // namespace utils

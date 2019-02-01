#pragma once

#include "net/SessionBase.hpp"
#include "net/core.hpp"
#include <api/datachannelinterface.h>
#include <cstdint>
#include <folly/ProducerConsumerQueue.h>
#include <rapidjson/document.h>
#include <rtc_base/criticalsection.h>
#include <string>
#include <vector>
#include <webrtc/api/peerconnectioninterface.h>
#include <webrtc/rtc_base/scoped_ref_ptr.h>

namespace cricket {
class BasicPortAllocator;
}

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
class PeerConnectivityChecker;

// NOTE: ProducerConsumerQueue must be created with a fixed maximum size
// We use Queue per connection
constexpr size_t MAX_SENDQUEUE_SIZE = 12;

/**
 * A class which represents a single connection
 * When this class is destroyed, the connection is closed.
 **/
class WRTCSession : public SessionBase, public std::enable_shared_from_this<WRTCSession> {
public:
  // WRTCSession() {} // TODO

  explicit WRTCSession(NetworkManager* nm, const std::string& webrtcId, const std::string& wsId);

  ~WRTCSession();

  void CloseDataChannel(NetworkManager* nm,
                        rtc::scoped_refptr<webrtc::DataChannelInterface>& in_data_channel,
                        rtc::scoped_refptr<webrtc::PeerConnectionInterface> pci_);

  // bool handleIncomingJSON(std::shared_ptr<std::string> message) override;

  void send(const std::string& ss) override;

  void setObservers();

  bool isExpired() const override;
  // bool isExpired() const override { return false; }

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

  bool isDataChannelOpen();

  void onDataChannelMessage(const webrtc::DataBuffer& buffer);

  void onDataChannelOpen();

  void onDataChannelClose();

  bool streamStillUsed(const std::string& streamLabel);

  static bool send(NetworkManager* nm, std::shared_ptr<WRTCSession> wrtcSess,
                   const std::string& data);

  static bool sendQueued(NetworkManager* nm, std::shared_ptr<WRTCSession> wrtcSess);

  // TODO private >>

  rtc::scoped_refptr<webrtc::PeerConnectionInterface> pci_; // TODO: private

  // The data channel used to communicate.
  rtc::scoped_refptr<webrtc::DataChannelInterface> dataChannelI_;

  // The socket that the signaling thread and worker thread communicate on.
  // CustomSocketServer socket_server;
  // rtc::PhysicalSocketServer socket_server;
  // last updated DataChannel state
  webrtc::DataChannelInterface::DataState lastDataChannelstate_;

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

  void setFullyCreated(bool isFullyCreated) { isFullyCreated_ = isFullyCreated; }

  void createPeerConnection();

  void createPeerConnectionObserver();

  webrtc::SessionDescriptionInterface* createSessionDescription(const std::string& type,
                                                                const std::string& sdp);

  bool InitializePortAllocator();

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

  // boost::asio::steady_timer timer_;

  std::unique_ptr<PeerConnectivityChecker> connectionChecker_;

  /**
   * If you want to send more than one message at a time, you need to implement
   * your own write queue.
   * @see https://github.com/boostorg/beast/issues/1207
   *
   * @note ProducerConsumerQueue is a one producer and one consumer queue
   * without locks.
   **/

  folly::ProducerConsumerQueue<std::shared_ptr<const std::string>> sendQueue_{MAX_SENDQUEUE_SIZE};

  std::unique_ptr<cricket::BasicPortAllocator> portAllocator_;

  bool enableEnumeratingAllNetworkInterfaces_{true};

  bool isSendBusy_{false};

  const uint64_t MAX_TO_BUFFER_BYTES{1024 * 1024};
};

} // namespace wrtc
} // namespace net
} // namespace gloer

#pragma once

#include "net/SessionBase.hpp"
#include "net/core.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include <api/datachannelinterface.h>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <folly/ProducerConsumerQueue.h>
#include <iostream>
#include <rapidjson/document.h>
#include <string>
#include <thread>
#include <vector>
#include <webrtc/api/peerconnectioninterface.h>
#include <webrtc/rtc_base/criticalsection.h>
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
constexpr size_t MAX_SENDQUEUE_SIZE = 120;

/**
 * A class which represents a single connection
 * When this class is destroyed, the connection is closed.
 **/
class WRTCSession : public SessionBase, public std::enable_shared_from_this<WRTCSession> {
public:
  WRTCSession() = delete;

  explicit WRTCSession(NetworkManager* nm, const std::string& webrtcId, const std::string& wsId)
      RTC_RUN_ON(thread_checker_);

  ~WRTCSession() override; // RTC_RUN_ON(thread_checker_);

  void CloseDataChannel(bool resetObserver) RTC_RUN_ON(signalingThread());

  // bool handleIncomingJSON(std::shared_ptr<std::string> message) override;

  void send(const std::string& ss) override; // RTC_RUN_ON(thread_checker_);

  void setObservers(bool isServer) RTC_RUN_ON(thread_checker_);

  bool isExpired() const override RTC_RUN_ON(signalingThread());
  // bool isExpired() const override { return false; }

  // rtc::scoped_refptr<webrtc::PeerConnectionInterface> getPCI() const;

  // rtc::scoped_refptr<webrtc::DataChannelInterface> getDataChannelI() const;

  void setLocalDescription(webrtc::SessionDescriptionInterface* sdi) RTC_RUN_ON(signalingThread());

  webrtc::DataChannelInterface::DataState updateDataChannelState()
      RTC_RUN_ON(lastStateMutex_); // RTC_RUN_ON(signaling_thread());

  void createAndAddIceCandidateFromJson(const rapidjson::Document& message_object)
      RTC_RUN_ON(signalingThread());

  // Triggered when a remote peer opens a data channel.
  void onDataChannelCreated(NetworkManager* nm,
                            rtc::scoped_refptr<webrtc::DataChannelInterface> channel)
      RTC_RUN_ON(signalingThread());

  static void
  onIceCandidate(NetworkManager* nm, const std::string& wsConnId,
                 const webrtc::IceCandidateInterface* candidate); // RTC_RUN_ON(signaling_thread());

  void onAnswerCreated(webrtc::SessionDescriptionInterface* desc) RTC_RUN_ON(signalingThread());

  bool IsStable();

  void onOfferCreated(webrtc::SessionDescriptionInterface* desc) RTC_RUN_ON(signalingThread());

  bool isDataChannelOpen() RTC_RUN_ON(lastStateMutex_); // RTC_RUN_ON(&thread_checker_);

  void onDataChannelMessage(const webrtc::DataBuffer& buffer) RTC_RUN_ON(signalingThread());

  void onDataChannelAllocated() RTC_RUN_ON(signalingThread());

  void onDataChannelDeallocated() RTC_RUN_ON(signalingThread());

  bool streamStillUsed(const std::string& streamLabel) RTC_RUN_ON(signalingThread());

  // queue sending
  static bool send(NetworkManager* nm, std::shared_ptr<WRTCSession> wrtcSess,
                   const std::string& data); // RTC_RUN_ON(thread_checker_);

  static bool
  sendQueued(NetworkManager* nm,
             std::shared_ptr<WRTCSession> wrtcSess); // RTC_GUARDED_BY(signaling_thread())

  // TODO private >>

  rtc::CriticalSection peerConIMutex_; // TODO: to private///
  rtc::scoped_refptr<webrtc::PeerConnectionInterface> pci_ RTC_GUARDED_BY(peerConIMutex_);

  // The data channel used to communicate.
  rtc::scoped_refptr<webrtc::DataChannelInterface> dataChannelI_;

  // The socket that the signaling thread and worker thread communicate on.
  // CustomSocketServer socket_server;
  // rtc::PhysicalSocketServer socket_server;
  // last updated DataChannel state
  rtc::CriticalSection lastStateMutex_;
  webrtc::DataChannelInterface::DataState lastDataChannelstate_ RTC_GUARDED_BY(lastStateMutex_);

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

  std::unique_ptr<DCO> tmp_dataChannelObserver_;

  // The observer that responds to session description creation events.
  // webrtc::CreateSessionDescriptionObserver for creating an offer or answer.
  rtc::scoped_refptr<CSDO> createSDO_;

  std::shared_ptr<PCO> peerConnectionObserver_;

  void createDCI() RTC_RUN_ON(signalingThread());

  void setRemoteDescription(webrtc::SessionDescriptionInterface* clientSessionDescription)
      RTC_RUN_ON(signalingThread());

  void CreateAnswer() RTC_RUN_ON(signalingThread());

  void CreateOffer() RTC_RUN_ON(signalingThread()); // TODO: use in client

  bool fullyCreated() const; // RTC_RUN_ON(FullyCreatedMutex_); // RTC_RUN_ON(signaling_thread());

  boost::posix_time::ptime lastRecievedMsgTime RTC_GUARDED_BY(signalingThread()) =
      boost::posix_time::second_clock::local_time();

  static const boost::posix_time::time_duration
      timerDeadlinePeriod; // RTC_GUARDED_BY(signaling_thread())

  boost::posix_time::ptime timerDeadline RTC_GUARDED_BY(signalingThread()) =
      boost::posix_time::second_clock::local_time() + timerDeadlinePeriod;

  void setFullyCreated(
      bool isFullyCreated); // RTC_RUN_ON(FullyCreatedMutex_); // RTC_RUN_ON(signaling_thread());

  void createPeerConnection() RTC_RUN_ON(signalingThread());

  void createPeerConnectionObserver() RTC_RUN_ON(signalingThread());

  webrtc::SessionDescriptionInterface* // std::unique_ptr<webrtc::SessionDescriptionInterface>
  createSessionDescription(const std::string& type, const std::string& sdp);

  bool InitializePortAllocator_n() RTC_RUN_ON(networkThread());

  rtc::Thread* networkThread() const;

  rtc::Thread* workerThread() const;

  rtc::Thread* signalingThread() const;

  void close_s(bool closePci, bool resetChannelObserver) RTC_RUN_ON(signalingThread());

  void setClosing(bool closing) RTC_RUN_ON(signalingThread());

  bool isClosing() const RTC_RUN_ON(signalingThread());

  void addDataChannelCount_s(uint32_t count) RTC_RUN_ON(signalingThread());

  void subDataChannelCount_s(uint32_t count) RTC_RUN_ON(signalingThread());

private:
  // rtc::CriticalSection FullyCreatedMutex_;
  // https://stackoverflow.com/questions/7223164/is-mutex-needed-to-synchronize-a-simple-flag-between-pthreads
  std::atomic<bool> isFullyCreated_ = false;
  // bool isFullyCreated_ RTC_GUARDED_BY(FullyCreatedMutex_) = false; // TODO AtomicBool

  /**
   * 16 Kbyte for the highest throughput, while also being the most portable one
   * @see viblast.com/blog/2015/2/5/webrtc-data-channel-message-size/
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
   * @see github.com/boostorg/beast/issues/1207
   *
   * @note ProducerConsumerQueue is a one producer and one consumer queue
   * without locks.
   **/

  ::folly::ProducerConsumerQueue<std::shared_ptr<const std::string>> sendQueue_{MAX_SENDQUEUE_SIZE};

  bool isClosing_ RTC_GUARDED_BY(signalingThread());

  std::unique_ptr<cricket::BasicPortAllocator> portAllocator_;

  bool enableEnumeratingAllNetworkInterfaces_{true};

  bool isSendBusy_ RTC_GUARDED_BY(signalingThread()) = false;

  const uint64_t MAX_TO_BUFFER_BYTES{1024 * 1024};

  uint32_t dataChannelCount_{0};

  // ThreadChecker is a helper class used to help verify that some methods of a
  // class are called from the same thread.
  rtc::ThreadChecker thread_checker_;

  uint16_t minPort_ = 0;

  uint16_t maxPort_ = 65535;

  RTC_DISALLOW_COPY_AND_ASSIGN(WRTCSession);
};

} // namespace wrtc
} // namespace net
} // namespace gloer

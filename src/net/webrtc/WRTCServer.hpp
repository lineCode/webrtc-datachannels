#pragma once

#include "algorithm/CallbackManager.hpp"
#include "algorithm/NetworkOperation.hpp"
#include <api/datachannelinterface.h>
#include <cstdint>
#include <rapidjson/document.h>
#include <rtc_base/criticalsection.h>
#include <string>
#include <vector>
#include <webrtc/api/peerconnectioninterface.h>
#include <webrtc/p2p/client/basicportallocator.h>
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
class WRTCSession;

struct WRTCNetworkOperation : public algo::NetworkOperation<algo::WRTC_OPCODE> {
  WRTCNetworkOperation(const algo::WRTC_OPCODE& operationCode, const std::string& operationName)
      : NetworkOperation(operationCode, operationName) {}

  WRTCNetworkOperation(const algo::WRTC_OPCODE& operationCode) : NetworkOperation(operationCode) {}
};

typedef std::function<void(utils::net::WRTCSession* clientSession, utils::net::NetworkManager* nm,
                           std::shared_ptr<std::string> messageBuffer)>
    WRTCNetworkOperationCallback;

class WRTCInputCallbacks
    : public algo::CallbackManager<WRTCNetworkOperation, WRTCNetworkOperationCallback> {
public:
  WRTCInputCallbacks();

  ~WRTCInputCallbacks();

  std::map<WRTCNetworkOperation, WRTCNetworkOperationCallback> getCallbacks() const override;

  void addCallback(const WRTCNetworkOperation& op, const WRTCNetworkOperationCallback& cb) override;
};

class WRTCServer {
public:
  WRTCServer(NetworkManager* nm);

  ~WRTCServer();

  ////////
  void sendToAll(const std::string& message);

  void sendTo(const std::string& sessionID, const std::string& message);

  void handleAllPlayerMessages();

  void doToAllSessions(std::function<void(std::shared_ptr<WRTCSession>)> func);

  /**
   * @brief returns the number of connected clients
   *
   * @return number of valid sessions
   */
  size_t getSessionsCount() const;

  std::unordered_map<std::string, std::shared_ptr<WRTCSession>> getSessions() const;

  std::shared_ptr<WRTCSession> getSessById(const std::string& sessionID);

  bool addSession(const std::string& sessionID, std::shared_ptr<WRTCSession> sess);

  void unregisterSession(const std::string& id);

  WRTCInputCallbacks getWRTCOperationCallbacks() const;
  ///////

  void Quit();

  void resetWebRtcConfig(const std::vector<webrtc::PeerConnectionInterface::IceServer>& iceServers);

  void InitAndRun();

  std::shared_ptr<algo::DispatchQueue> getWRTCQueue() const { return WRTCQueue_; };

  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> getPCF() const;

  webrtc::PeerConnectionInterface::RTCConfiguration getWRTCConf() const;

  // The peer connection through which we engage in the Session Description
  // Protocol (SDP) handshake.
  rtc::CriticalSection pcMutex_; // TODO: to private

  // TODO: global config var
  webrtc::DataChannelInit dataChannelConf_; // TODO: to private

  webrtc::PeerConnectionInterface::RTCOfferAnswerOptions webrtcGamedataOpts_; // TODO: to private

  void addDataChannelCount(uint32_t count);

  void subDataChannelCount(uint32_t count);

  // see https://github.com/sourcey/libsourcey/blob/master/src/webrtc/include/scy/webrtc/peer.h
  // see https://github.com/sourcey/libsourcey/blob/master/src/webrtc/src/peer.cpp
  std::unique_ptr<cricket::BasicPortAllocator> portAllocator_;

  // see
  // https://github.com/sourcey/libsourcey/blob/master/src/webrtc/include/scy/webrtc/peerfactorycontext.h
  // see https://github.com/sourcey/libsourcey/blob/master/src/webrtc/src/peerfactorycontext.cpp
  std::unique_ptr<rtc::NetworkManager> networkManager_;
  std::unique_ptr<rtc::PacketSocketFactory> socketFactory_;

private:
  std::shared_ptr<algo::DispatchQueue> WRTCQueue_; // uses parent thread (same thread)

  // Used to map WRTCSessionId to WRTCSession
  std::unordered_map<std::string, std::shared_ptr<WRTCSession>> sessions_;

  /*rtc::scoped_refptr<webrtc::PeerConnectionInterface>
      peerConnection_;*/

  // TODO: weak ptr
  NetworkManager* nm_;

  // thread for WebRTC listening loop.
  // TODO
  // std::thread webrtc_thread;

  webrtc::PeerConnectionInterface::RTCConfiguration webrtcConf_;

  /*
   * The signaling thread handles the bulk of WebRTC computation;
   * it creates all of the basic components and fires events we can consume by
   * calling the observer methods
   */
  std::unique_ptr<rtc::Thread> signalingThread_;

  std::unique_ptr<rtc::Thread> networkThread_;
  /*
   * worker thread, on the other hand, is delegated resource-intensive tasks
   * such as media streaming to ensure that the signaling thread doesn’t get
   * blocked
   */
  std::unique_ptr<rtc::Thread> workerThread_;

  // rtc::Thread* network_thread_;
  // The peer conncetion factory that sets up signaling and worker threads. It
  // is also used to create the PeerConnection.
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peerConnectionFactory_;

  uint32_t dataChannelCount_; // TODO
  // TODO: close data_channel on timer?
  // uint32_t getMaxSessionId() const { return maxSessionId_; }
  // TODO: limit max num of open sessions
  // uint32_t maxSessionId_ = 0;
  // TODO: limit max num of open connections per IP
  // uint32_t maxConnectionsPerIP_ = 0;

  WRTCInputCallbacks wrtcOperationCallbacks_;
};

} // namespace net
} // namespace utils

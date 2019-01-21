#pragma once

#include "algorithm/CallbackManager.hpp"
#include "algorithm/NetworkOperation.hpp"
#include "net/SessionManagerI.hpp"
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
namespace config {
class ServerConfig;
} // namespace config
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

typedef std::function<void(WRTCSession* clientSession, NetworkManager* nm,
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

class WRTCServer : public SessionManagerI<WRTCSession, WRTCInputCallbacks> {
public:
  WRTCServer(NetworkManager* nm, const utils::config::ServerConfig& serverConfig);

  ~WRTCServer();

  void sendToAll(const std::string& message) override;

  void sendTo(const std::string& sessionID, const std::string& message) override;

  void handleAllPlayerMessages() override;

  void unregisterSession(const std::string& id) override;

  void runThreads(const utils::config::ServerConfig& serverConfig) override;

  void finishThreads() override;

  // The thread entry point for the WebRTC thread. This sets the WebRTC thread as
  // the signaling thread and creates a worker thread in the background.
  void webRtcSignalThreadEntry();

  void resetWebRtcConfig(const std::vector<webrtc::PeerConnectionInterface::IceServer>& iceServers);

  void InitAndRun();

  // std::shared_ptr<algo::DispatchQueue> getWRTCQueue() const { return WRTCQueue_; };

  // rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> getPCF() const;

  webrtc::PeerConnectionInterface::RTCConfiguration getWRTCConf() const;

  void addDataChannelCount(uint32_t count);

  void subDataChannelCount(uint32_t count);

public:
  std::thread webrtcThread_;

  // The peer connection through which we engage in the Session Description
  // Protocol (SDP) handshake.
  rtc::CriticalSection pcMutex_; // TODO: to private

  // TODO: global config var
  webrtc::DataChannelInit dataChannelConf_; // TODO: to private

  webrtc::PeerConnectionInterface::RTCOfferAnswerOptions webrtcGamedataOpts_; // TODO: to private

  // see https://github.com/sourcey/libsourcey/blob/master/src/webrtc/include/scy/webrtc/peer.h
  // see https://github.com/sourcey/libsourcey/blob/master/src/webrtc/src/peer.cpp
  std::unique_ptr<cricket::BasicPortAllocator> portAllocator_;

  // see
  // https://github.com/sourcey/libsourcey/blob/master/src/webrtc/include/scy/webrtc/peerfactorycontext.h
  // see https://github.com/sourcey/libsourcey/blob/master/src/webrtc/src/peerfactorycontext.cpp
  std::unique_ptr<rtc::NetworkManager> networkManager_;

  std::unique_ptr<rtc::PacketSocketFactory> socketFactory_;

  // rtc::Thread* network_thread_;
  // The peer conncetion factory that sets up signaling and worker threads. It
  // is also used to create the PeerConnection.
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peerConnectionFactory_;

private:
  // std::shared_ptr<algo::DispatchQueue> WRTCQueue_; // uses parent thread (same thread)

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

  uint32_t dataChannelCount_; // TODO
  // TODO: close data_channel on timer?
  // uint32_t getMaxSessionId() const { return maxSessionId_; }
  // TODO: limit max num of open sessions
  // uint32_t maxSessionId_ = 0;
  // TODO: limit max num of open connections per IP
  // uint32_t maxConnectionsPerIP_ = 0;
};

} // namespace net
} // namespace utils

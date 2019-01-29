#pragma once

#include "algo/CallbackManager.hpp"
#include "algo/NetworkOperation.hpp"
#include "net/SessionManagerBase.hpp"
#include <api/datachannelinterface.h>
#include <cstdint>
#include <net/core.hpp>
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

namespace gloer {
namespace algo {
class DispatchQueue;
} // namespace algo
} // namespace gloer

namespace gloer {
namespace config {
struct ServerConfig;
} // namespace config
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

class WRTCServer : public SessionManagerBase<WRTCSession, WRTCInputCallbacks> {
public:
  WRTCServer(NetworkManager* nm, const gloer::config::ServerConfig& serverConfig);

  ~WRTCServer();

  void sendToAll(const std::string& message) override;

  void sendTo(const std::string& sessionID, const std::string& message) override;

  void handleIncomingMessages() override;

  void unregisterSession(const std::string& id) override;

  void runThreads(const gloer::config::ServerConfig& serverConfig) override;

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

  // creates WRTCSession based on WebSocket message
  static void setRemoteDescriptionAndCreateAnswer(ws::WsSession* clientWsSession,
                                                  NetworkManager* nm,
                                                  const rapidjson::Document& message_object);

public:
  std::thread webrtcThread_;

  // The peer connection through which we engage in the Session Description
  // Protocol (SDP) handshake.
  rtc::CriticalSection pcMutex_; // TODO: to private

  // TODO: global config var
  webrtc::DataChannelInit dataChannelConf_; // TODO: to private

  webrtc::PeerConnectionInterface::RTCOfferAnswerOptions webrtcGamedataOpts_; // TODO: to private

  // see
  // https://github.com/sourcey/libsourcey/blob/master/src/webrtc/include/scy/webrtc/peerfactorycontext.h
  // see https://github.com/sourcey/libsourcey/blob/master/src/webrtc/src/peerfactorycontext.cpp
  std::unique_ptr<rtc::NetworkManager> wrtcNetworkManager_;

  std::unique_ptr<rtc::PacketSocketFactory> socketFactory_;

  // rtc::Thread* network_thread_;
  // The peer conncetion factory that sets up signaling and worker threads. It
  // is also used to create the PeerConnection.
  static rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peerConnectionFactory_;

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

} // namespace wrtc
} // namespace net
} // namespace gloer

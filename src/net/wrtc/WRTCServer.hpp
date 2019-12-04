#pragma once

#include "algo/CallbackManager.hpp"
#include "algo/NetworkOperation.hpp"
#include "net/wrtc/SessionManager.hpp"
#include <api/datachannelinterface.h>
#include <cstdint>
#include <net/wrtc/Callbacks.hpp>
#include <rapidjson/document.h>
#include <string>
#include <vector>
#include <webrtc/api/peerconnectioninterface.h>
#include <webrtc/api/test/fakeconstraints.h>
#include <webrtc/p2p/client/basicportallocator.h>
#include <webrtc/rtc_base/asyncinvoker.h>
#include <webrtc/rtc_base/callback.h>
#include <webrtc/rtc_base/criticalsection.h>
#include <webrtc/rtc_base/messagehandler.h>
#include <webrtc/rtc_base/messagequeue.h>
#include <webrtc/rtc_base/scoped_ref_ptr.h>
#include <webrtc/rtc_base/ssladapter.h>
#include <webrtc/rtc_base/thread.h>
#include "net/ConnectionManagerBase.hpp"
#include "net/wrtc/SessionManager.hpp"
#include "net/wrtc/Callbacks.hpp"

//#include <webrtc/base/single_thread_task_runner.h>
//#include <webrtc/base/task_runner.h>
//#include <webrtc/base/threading/thread.h>
//#include <webrtc/rtc_base/bind.h>
//#include <webrtc/rtc_base/location.h>

/*
  StunRequest();
  explicit StunRequest(StunMessage* request);
  ~StunRequest() override;

  // Handles messages for sending and timeout.
  void OnMessage(rtc::Message* pmsg) override;
*/

/*template <typename F> struct OnceFunctorHelper : rtc::MessageHandler {
  OnceFunctorHelper() {}
  OnceFunctorHelper(F functor) : functor_(functor) {}
  ~OnceFunctorHelper() override {}

  void OnMessage(rtc::Message* ) override {
    functor_();
    delete this;
  }

  F functor_;
};*/

/*template <typename F> rtc::FunctorMessageHandler<void, F>* OnceFunctor(F functor) {
  return new rtc::FunctorMessageHandler<void, F>(functor); // OnceFunctorHelper<F>(functor);
}*/

/**
 * @see base/task_runner.cc
 * @see chromium/components/devtools_bridge/session_dependency_factory.cc
 * Posts tasks on signaling thread. If stopped (when SesseionDependencyFactry
 * is destroying) ignores posted tasks.
 */
/*class SignalingThreadTaskRunner : public ::base::TaskRunner, private rtc::MessageHandler {
public:
  explicit SignalingThreadTaskRunner(rtc::Thread* thread) : thread_(thread) {}

  bool PostDelayedTask(const base::Location& from_here, const base::Closure& task,
                       base::TimeDelta delay) override {
    DCHECK(delay.ToInternalValue() == 0);

    rtc::CritScope scope(&critical_section_);

    if (thread_)
      thread_->Send(this, 0, new Task(task));

    return true;
  }

  bool RunsTasksOnCurrentThread() const override {
    rtc::CritScope scope(&critical_section_);

    return thread_ != NULL && thread_->IsCurrent();
  }

  void Stop() {
    rtc::CritScope scope(&critical_section_);
    thread_ = NULL;
  }

private:
  typedef rtc::TypedMessageData<base::Closure> Task;

  ~SignalingThreadTaskRunner() override {}

  void OnMessage(rtc::Message* msg) override { static_cast<Task*>(msg->pdata)->data().Run(); }

  mutable rtc::CriticalSection critical_section_;
  rtc::Thread* thread_; // Guarded by |critical_section_|.
};*/

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

class SessionPair;

namespace ws {
class WsSession;
}

} // namespace net
} // namespace gloer

namespace gloer {
namespace net {
namespace wrtc {
class WRTCSession;

class WRTCServer : public ConnectionManagerBase {
public:
  WRTCServer(NetworkManager* nm, const gloer::config::ServerConfig& serverConfig, wrtc::SessionManager& sm);

  ~WRTCServer();

  void sendToAll(const std::string& message) override;

  void sendTo(const std::string& sessionID, const std::string& message) override;

  void runThreads_t(const gloer::config::ServerConfig& serverConfig) override
      RTC_RUN_ON(thread_checker_);

  void finishThreads_t() override RTC_RUN_ON(thread_checker_);

  // The thread entry point for the WebRTC thread.
  // This creates a worker/signaling/network thread in the background.
  void webRtcBackgroundThreadEntry();

  void
  resetWebRtcConfig_t(const std::vector<webrtc::PeerConnectionInterface::IceServer>& iceServers)
      RTC_RUN_ON(thread_checker_);

  void InitAndRun_t() RTC_RUN_ON(thread_checker_);

  // std::shared_ptr<algo::DispatchQueue> getWRTCQueue() const { return WRTCQueue_; };

  // rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> getPCF() const;

  webrtc::PeerConnectionInterface::RTCConfiguration getWRTCConf() const;

  void addGlobalDataChannelCount_s(uint32_t count) RTC_RUN_ON(signalingThread());

  void subGlobalDataChannelCount_s(uint32_t count) RTC_RUN_ON(signalingThread());

  // creates WRTCSession based on WebSocket message
  static void
  setRemoteDescriptionAndCreateAnswer(std::shared_ptr<SessionPair> clientWsSession,
                                      NetworkManager* nm,
                                      const std::string& sdp); // RTC_RUN_ON(signaling_thread());

  static std::shared_ptr<WRTCSession>
  setRemoteDescriptionAndCreateOffer(std::shared_ptr<SessionPair> clientWsSession,
                                     NetworkManager* nm); // RTC_RUN_ON(signaling_thread());

  rtc::Thread* startThread();

  rtc::Thread* signalingThread();

  rtc::Thread* workerThread();

  rtc::Thread* networkThread();

public:
  // std::thread webrtcStartThread_; // we create separate threads for wrtc

  // TODO: global config var
  webrtc::DataChannelInit dataChannelConf_; // TODO: to private

  webrtc::PeerConnectionInterface::RTCOfferAnswerOptions webrtcGamedataOpts_; // TODO: to private

  // @see
  // github.com/sourcey/libsourcey/blob/master/src/webrtc/include/scy/webrtc/peerfactorycontext.h
  // @see github.com/sourcey/libsourcey/blob/master/src/webrtc/src/peerfactorycontext.cpp
  std::unique_ptr<rtc::NetworkManager> wrtcNetworkManager_;

  std::unique_ptr<rtc::PacketSocketFactory> socketFactory_;

  // The peer connection through which we engage in the Session Description
  // Protocol (SDP) handshake.
  rtc::CriticalSection pcfMutex_; // TODO: to private
  // @see
  // chromium.googlesource.com/external/webrtc/stable/talk/+/master/app/webrtc/peerconnectioninterface.h
  // The peer conncetion factory that sets up signaling and worker threads. It
  // is also used to create the PeerConnection.
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
      peerConnectionFactory_ RTC_GUARDED_BY(pcfMutex_);

  /*
   * The signaling thread handles the bulk of WebRTC computation;
   * it creates all of the basic components and fires events we can consume by
   * calling the observer methods
   * Calls to the Stream APIs (mediastreaminterface.h) and the PeerConnection APIs
   * (peerconnectioninterface.h) will be proxied to the signaling thread, which means that an
   * application can call those APIs from whatever thread. All callbacks will be made on the
   * signaling thread. The application should return the callback as quickly as possible to avoid
   * blocking the signaling thread. Resource-intensive processes should be posted to a different
   * thread.
   */
  rtc::Thread* startThread_ = nullptr;

  std::unique_ptr<rtc::AsyncInvoker> asyncInvoker_;

  std::unique_ptr<rtc::Thread> owned_signalingThread_;
  rtc::Thread* signaling_thread_ = nullptr;

  std::unique_ptr<rtc::Thread> owned_networkThread_;
  rtc::Thread* network_thread_ = nullptr;
  /*
   * The worker thread is delegated resource-intensive tasks
   * such as media streaming to ensure that the signaling thread doesn’t get
   * blocked
   */
  std::unique_ptr<rtc::Thread> owned_workerThread_;
  rtc::Thread* worker_thread_ = nullptr;

  static std::string sessionDescriptionStrFromJson(const rapidjson::Document& message_object);

  static std::shared_ptr<WRTCSession>
  createNewSession(bool isServer, std::shared_ptr<SessionPair> clientWsSession,
                   NetworkManager* nm); // RTC_RUN_ON(signaling_thread());

  void addCallback(const WRTCNetworkOperation& op, const WRTCNetworkOperationCallback& cb);

private:
  // std::shared_ptr<algo::DispatchQueue> WRTCQueue_; // uses parent thread (same thread)

  /*rtc::scoped_refptr<webrtc::PeerConnectionInterface>
      peerConnection_;*/

  // TODO: weak ptr
  NetworkManager* nm_;

  wrtc::SessionManager& sm_;

  // thread for WebRTC listening loop.
  // TODO
  // std::thread webrtc_thread;

  webrtc::PeerConnectionInterface::RTCConfiguration webrtcConf_;

  uint32_t dataChannelGlobalCount_ RTC_GUARDED_BY(signalingThread()) = 0;
  // TODO: close data_channel on timer?
  // uint32_t getMaxSessionId() const { return maxSessionId_; }
  // TODO: limit max num of open sessions
  // uint32_t maxSessionId_ = 0;
  // TODO: limit max num of open connections per IP
  // uint32_t maxConnectionsPerIP_ = 0;

  bool networkThreadWithSocketServer_{true};

  // ThreadChecker is a helper class used to help verify that some methods of a
  // class are called from the same thread.
  rtc::ThreadChecker thread_checker_;

  RTC_DISALLOW_COPY_AND_ASSIGN(WRTCServer);
};

} // namespace wrtc
} // namespace net
} // namespace gloer

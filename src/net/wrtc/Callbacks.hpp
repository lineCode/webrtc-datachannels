#pragma once

#include "algo/CallbackManager.hpp"
#include "algo/NetworkOperation.hpp"
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
#include "net/NetworkManagerBase.hpp"

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

template <typename F> struct OnceFunctorHelper : rtc::MessageHandler {
  explicit OnceFunctorHelper(F functor) : functor_(functor) {}

  void OnMessage(rtc::Message* /*msg*/) override {
    functor_();
    delete this;
  }

  F functor_;
};

template <typename F> rtc::MessageHandler* OnceFunctor(F functor) {
  return new OnceFunctorHelper<F>(functor);
}

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

//class NetworkManager;

class SessionPair;

namespace ws {
class ServerSession;
}

} // namespace net
} // namespace gloer

namespace gloer {
namespace net {
namespace wrtc {
class WRTCSession;
struct WRTCNetworkOperation : public algo::NetworkOperation<algo::WRTC_OPCODE> {
  explicit WRTCNetworkOperation(const algo::WRTC_OPCODE& operationCode,
                                const std::string& operationName)
      : NetworkOperation(operationCode, operationName) {}

  explicit WRTCNetworkOperation(const algo::WRTC_OPCODE& operationCode)
      : NetworkOperation(operationCode) {}
};

typedef std::function<void(std::shared_ptr<WRTCSession> clientSession, net::WRTCNetworkManager* nm,
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

} // namespace wrtc
} // namespace net
} // namespace gloer

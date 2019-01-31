#pragma once

#include <functional>

#include "net/SessionManagerBase.hpp"
#include <api/datachannelinterface.h>
#include <cstdint>
#include <net/NetworkManager.hpp>
#include <net/core.hpp>
#include <rapidjson/document.h>
#include <rtc_base/criticalsection.h>
#include <string>
#include <vector>
#include <webrtc/api/peerconnectioninterface.h>
#include <webrtc/p2p/client/basicportallocator.h>
#include <webrtc/rtc_base/bind.h>
#include <webrtc/rtc_base/messagehandler.h>
#include <webrtc/rtc_base/scoped_ref_ptr.h>
#include <webrtc/rtc_base/thread.h>

namespace gloer {
namespace net {
namespace wrtc {

/*
template <typename F> struct OnceFunctorHelper : rtc::MessageHandler {
  OnceFunctorHelper(F functor) : functor_(functor) {}

  void OnMessage(rtc::Message* ) override {
  functor_();
  delete this;
}

F functor_;
};
*/

/* public:
  virtual ~MessageHandler();
  virtual void OnMessage(Message* msg) = 0;

 protected:
  MessageHandler() {}*/

class Timer /*: rtc::MessageHandler*/ {
public:
  Timer(NetworkManager* nm);
  ~Timer();

  void start(int intervalMs, std::function<void()> callback);
  void singleShot(int delay, std::function<void()> callback);
  bool started() const;
  void stop();

protected:
  // virtual void OnMessage(rtc::Message* msg) override;
  void OnTimer();

  int _interval;
  std::function<void()> _callback;
  bool _singleShot{false};
  NetworkManager* nm_;
  bool _needStop{false};

  RTC_DISALLOW_COPY_AND_ASSIGN(Timer);
};

} // namespace wrtc
} // namespace net
} // namespace gloer

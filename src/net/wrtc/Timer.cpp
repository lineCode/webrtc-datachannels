#include "Timer.h" // IWYU pragma: associated

#include "log/Logger.hpp"
#include "net/SessionManagerBase.hpp"
#include <api/datachannelinterface.h>
#include <cstdint>
#include <net/core.hpp>
#include <net/wrtc/WRTCServer.hpp>
#include <rapidjson/document.h>
#include <rtc_base/criticalsection.h>
#include <string>
#include <vector>
#include <webrtc/api/peerconnectioninterface.h>
#include <webrtc/p2p/client/basicportallocator.h>
#include <webrtc/rtc_base/bind.h>
#include <webrtc/rtc_base/scoped_ref_ptr.h>
#include <webrtc/rtc_base/thread.h>

//#ifdef WEBRTC_POSIX /* dirty fix for linker errors */
// namespace rtc {
//// MessageQueueManager does cleanup of of message queues
// MessageHandler::~MessageHandler() { MessageQueueManager::Clear(this); }
//} // namespace rtc
//#endif

namespace gloer {
namespace net {
namespace wrtc {

Timer::Timer(NetworkManager* nm) : nm_(nm), _interval(0) {}

Timer::~Timer() /*: ~MessageHandler()*/ { stop(); }

void Timer::start(int intervalMs, std::function<void()> callback) {
  stop();
  _interval = intervalMs;
  _callback = callback;
  _singleShot = false;
  // rtc::Thread::Current()->PostDelayed(RTC_FROM_HERE, _interval, this);

  {
    auto nm = nm_;
    auto handle = OnceFunctor([this]() { OnTimer(); });
    nm->getWRTC()->workerThread_->PostDelayed(RTC_FROM_HERE, _interval, handle);
  }
}

void Timer::singleShot(int delay, std::function<void()> callback) {
  stop();
  _callback = callback;
  _singleShot = true;
  // rtc::Thread::Current()->PostDelayed(RTC_FROM_HERE, delay, this);
  {
    auto nm = nm_;
    auto handle = OnceFunctor([this]() { OnTimer(); });
    nm->getWRTC()->workerThread_->PostDelayed(RTC_FROM_HERE, delay, handle);
  }
}

bool Timer::started() const { return static_cast<bool>(_callback); }

void Timer::stop() {
  _callback = std::function<void()>();
  _needStop = true;

  // From MessageQueue
  // rtc::Thread::Current()->Clear(this);
}

void Timer::OnTimer() {
  if (_callback) {
    _callback();
    if (_singleShot) {
      /* reset _callback, to make started() return false after this callback */
      _callback = std::function<void()>();
    } else if (!_needStop) {
      // rtc::Thread::Current()->PostDelayed(RTC_FROM_HERE, _interval, this);
      {
        auto nm = nm_;
        auto handle = OnceFunctor([this]() { OnTimer(); });
        nm->getWRTC()->workerThread_->PostDelayed(RTC_FROM_HERE, _interval, handle);
      }
    }
  }
}

} // namespace wrtc
} // namespace net
} // namespace gloer

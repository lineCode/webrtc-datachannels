#include "PeerConnectivityChecker.h" // IWYU pragma: associated
#include "log/Logger.hpp"
#include <net/NetworkManager.hpp>
#include <net/wrtc/WRTCSession.hpp>

#include <webrtc/rtc_base/messagequeue.h>
#include <webrtc/rtc_base/thread.h>

namespace gloer {
namespace net {
namespace wrtc {

PeerConnectivityChecker::PeerConnectivityChecker(
    NetworkManager* nm, std::shared_ptr<WRTCSession> keepAliveSess,
    rtc::scoped_refptr<webrtc::DataChannelInterface> dc, ConnectivityLostCallback cb)
    : dataChannelI_(dc), connectLostCallback_(cb), keepAliveSess_(keepAliveSess), nm_(nm) {
  _timerStartTime = std::chrono::steady_clock::now();

  // starts periodic timer to check connectivity
  connectCheckDelayTimer_.start(connectCheckIntervalMs_,
                                std::bind(&PeerConnectivityChecker::_checkConnectivity, this));

  // starts periodic timer to send ping message
  pingStartDelayTimer_.singleShot(connectPingStartDelayTimeMs_,
                                  std::bind(&PeerConnectivityChecker::_startPing, this));
}

bool PeerConnectivityChecker::onRemoteActivity(const std::string& data) {
  if (data == PongMessage) {
    // got pong message from client, connection is alive
    _lastReceivedPongTime = std::chrono::steady_clock::now();
    return true;
  }
  // got some message from client, connection is alive
  _lastReceivedDataTime = std::chrono::steady_clock::now();
  return false;
}

void PeerConnectivityChecker::_startPing() {
  LOG(INFO) << "PeerConnectivityChecker: pingTimer start";
  pingTimer_.start(connectPingIntervalMs_, std::bind(&PeerConnectivityChecker::_sendPing, this));
}

void PeerConnectivityChecker::_sendPing() {
  // TODO: use WRTCSess instead of dataChannelI_, call WRTCSession::sendDataViaDataChannel
  if (dataChannelI_.get()) {
    dataChannelI_->Send(
        webrtc::DataBuffer(rtc::CopyOnWriteBuffer(PingMessage, sizeof(PingMessage)), true));
    _lastSentPingTime = std::chrono::steady_clock::now();
  }
}

void PeerConnectivityChecker::close() {
  pingStartDelayTimer_.stop();
  pingTimer_.stop();
  connectCheckDelayTimer_.stop();
}

void PeerConnectivityChecker::_checkConnectivity() {
  LOG(WARNING) << "PeerConnectivityChecker::_checkConnectivity";
  auto connectionLostAssumptionTime =
      std::chrono::steady_clock::now() - std::chrono::milliseconds(connectTimeoutMs_);

  bool assumeConnectivityLost = true;

  /*
   * check for uninitialized time values right after the connectivityChecker
   * is started.
   * call the callback, when after connectionLostAssumptionTime milliseconds,
   * if the timer values where not set yet.
   */
  if (!_lastReceivedDataTime && !_lastReceivedPongTime &&
      _timerStartTime > connectionLostAssumptionTime) {
    assumeConnectivityLost = false;
  }

  /*
   * check the time of the last received data:
   * if data was received after connectionLostAssumptionTime,
   * no connection loss is assumed.
   */
  if (_lastReceivedDataTime && _lastReceivedDataTime > connectionLostAssumptionTime) {
    assumeConnectivityLost = false;
  }

  /*
   * ... the same applies for explicit ping pong messages.
   */
  if (_lastReceivedPongTime && _lastReceivedPongTime > connectionLostAssumptionTime) {
    assumeConnectivityLost = false;
  }

  if (assumeConnectivityLost) {
    LOG(WARNING) << "PeerConnectivityChecker: connectivity probably lost";
    const bool needStop = connectLostCallback_();
    if (needStop) {
      close();
      if (keepAliveSess_.get()) {
        keepAliveSess_->close_s(false, false);
        // TODO: crash >>>>
        // keepAliveSess_ = nullptr; // free parent session
      }
    }
  }
}

} // namespace wrtc
} // namespace net
} // namespace gloer

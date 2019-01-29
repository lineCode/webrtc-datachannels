#pragma once

#include <chrono>
#include <functional>
#include <optional>

#include <webrtc/api/datachannelinterface.h>

#include "Timer.h"

namespace gloer {
namespace net {
namespace wrtc {

class PeerConnectivityChecker {
public:
  typedef std::function<void()> ConnectivityLostCallback;

  static constexpr const char* PingMessage = "CHECK_PING";
  static constexpr const char* PongMessage = "CHECK_PONG";

public:
  PeerConnectivityChecker(rtc::scoped_refptr<webrtc::DataChannelInterface> dc,
                          ConnectivityLostCallback cb);

  bool onRemoteActivity(const std::string& data);

protected:
  void _startPing();

  void _sendPing();

  void _checkConnectivity();

protected:
  rtc::scoped_refptr<webrtc::DataChannelInterface> dataChannelI_;

  ConnectivityLostCallback connectLostCallback_;

  // delay before starting to send ping messages
  Timer pingStartDelayTimer_;

  Timer pingTimer_;

  // delay before starting to check for connection lost
  Timer connectCheckDelayTimer_;

  std::optional<std::chrono::steady_clock::time_point> _timerStartTime;

  std::optional<std::chrono::steady_clock::time_point> _lastSentPingTime;

  std::optional<std::chrono::steady_clock::time_point> _lastReceivedPongTime;

  std::optional<std::chrono::steady_clock::time_point> _lastReceivedDataTime;

  int connectTimeoutMs_{10000};

  int connectCheckIntervalMs_{1000};

  int connectPingStartDelayTimeMs_{5000};

  int connectPingIntervalMs_{500};
};

} // namespace wrtc
} // namespace net
} // namespace gloer

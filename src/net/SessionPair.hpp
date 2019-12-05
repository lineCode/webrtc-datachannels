#pragma once

#include "net/SessionBase.hpp"
#include <net/ws/SessionGUID.hpp>

namespace gloer {
namespace net {
namespace wrtc {
class WRTCSession;
} // namespace wrtc
} // namespace net
} // namespace gloer

namespace gloer {
namespace net {

class SessionPair : public SessionBase<ws::SessionGUID> {
public:
  SessionPair(const ws::SessionGUID& id);

  virtual ~SessionPair() {}

  virtual bool isOpen() const = 0;

  virtual void close() = 0;

  virtual void pairToWRTCSession(std::shared_ptr<wrtc::WRTCSession> WRTCSession) = 0;

  virtual bool hasPairedWRTCSession() = 0;

  virtual std::weak_ptr<wrtc::WRTCSession> getWRTCSession() const = 0;
};

} // namespace net
} // namespace gloer

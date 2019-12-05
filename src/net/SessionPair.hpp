#pragma once

#include "net/SessionBase.hpp"
#include "log/Logger.hpp"
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <thread>
#include <vector>
#include <net/ws/SessionGUID.hpp>
//#include <net/wrtc/SessionGUID.hpp>

namespace gloer {
namespace config {
struct ServerConfig;
} // namespace config
} // namespace gloer

namespace gloer {
namespace algo {
class DispatchQueue;
} // namespace algo
} // namespace gloer

namespace gloer {
namespace net {
namespace wrtc {
class WRTCServer;
class WRTCSession;
} // namespace wrtc
} // namespace net
} // namespace gloer

namespace gloer {
namespace net {
namespace ws {
//class SessionGUID;
} // namespace ws
} // namespace net
} // namespace gloer

namespace gloer {
namespace net {

// typedef std::function<void(void)> expire_callback;

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

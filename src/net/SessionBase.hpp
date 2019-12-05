#pragma once

#include "log/Logger.hpp"
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

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

// typedef std::function<void(void)> expire_callback;

template<typename session_type>
class SessionBase {
public:
  typedef std::function<void(const session_type& sessId, const std::string& message)>
      on_message_callback;

  typedef std::function<void(const session_type& sessId)> on_close_callback;

  typedef uint32_t metadata_key;

  SessionBase(const session_type& id) : id_(id) {}

  virtual ~SessionBase() {}

  virtual void send(const std::string& ss) = 0;

  // virtual bool handleIncomingJSON(const std::shared_ptr<session_type> message) = 0;

  virtual session_type getId() const { return id_; }

  virtual bool isExpired() const = 0;

  // virtual void setExpiredCallback(); // TODO

  virtual void SetOnMessageHandler(on_message_callback handler) { onMessageCallback_ = handler; }

  virtual void SetOnCloseHandler(on_close_callback handler) { onCloseCallback_ = handler; }

protected:
  const session_type id_;

  //std::map<metadata_key, std::unique_ptr<MetaData>> metadata_;

  on_message_callback onMessageCallback_;

  on_close_callback onCloseCallback_;

  const size_t MAX_ID_LEN = 2048;
};

} // namespace net
} // namespace gloer

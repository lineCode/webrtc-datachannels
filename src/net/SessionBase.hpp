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

// typedef std::function<void(void)> expire_callback;

class SessionBase {
public:
  typedef std::function<void(const std::string& sessId, const std::string& message)>
      on_message_callback;

  typedef std::function<void(const std::string& sessId)> on_close_callback;

  SessionBase(const std::string& id);

  virtual ~SessionBase() {}

  virtual void send(const std::string& ss) = 0;

  // virtual bool handleIncomingJSON(const std::shared_ptr<std::string> message) = 0;

  virtual std::string getId() const { return id_; }

  virtual bool isExpired() const = 0;

  // virtual void setExpiredCallback(); // TODO

  virtual void SetOnMessageHandler(on_message_callback handler) { onMessageCallback_ = handler; }

  virtual void SetOnCloseHandler(on_close_callback handler) { onCloseCallback_ = handler; }

protected:
  const std::string id_;

  on_message_callback onMessageCallback_;

  on_close_callback onCloseCallback_;
};

} // namespace net
} // namespace gloer

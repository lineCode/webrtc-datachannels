#pragma once

#include "log/Logger.hpp"
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

namespace gloer {
namespace config {
class ServerConfig;
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
  SessionBase(const std::string& id);

  virtual ~SessionBase() {}

  virtual void send(std::shared_ptr<std::string> ss) = 0;

  virtual void send(const std::string& ss) = 0;

  virtual std::shared_ptr<algo::DispatchQueue> getReceivedMessages() const;

  virtual bool handleIncomingJSON(const std::shared_ptr<std::string> message) = 0;

  virtual std::string getId() const { return id_; }

  virtual bool hasReceivedMessages() const;

  virtual bool isExpired() const = 0;

  // virtual void setExpiredCallback(); // TODO

protected:
  const std::string id_;

  std::shared_ptr<algo::DispatchQueue> receivedMessagesQueue_;
};

} // namespace net
} // namespace gloer

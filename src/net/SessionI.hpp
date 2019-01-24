#pragma once

#include "log/Logger.hpp"
#include <boost/asio.hpp>
#include <memory>
#include <thread>
#include <vector>

namespace gloer {
namespace config {
class ServerConfig;
} // namespace config
} // namespace utils

namespace gloer {
namespace algo {
class DispatchQueue;
} // namespace algo
} // namespace utils

namespace gloer {
namespace net {

class SessionI {
public:
  SessionI(const std::string& id);

  virtual ~SessionI() {}

  virtual void send(std::shared_ptr<std::string> ss) = 0;

  virtual void send(const std::string& ss) = 0;

  virtual std::shared_ptr<algo::DispatchQueue> getReceivedMessages() const;

  virtual bool handleIncomingJSON(const std::shared_ptr<std::string> message) = 0;

  virtual std::string getId() const { return id_; }

  virtual bool hasReceivedMessages() const;

protected:
  const std::string id_;

  std::shared_ptr<algo::DispatchQueue> receivedMessagesQueue_;
};

} // namespace net
} // namespace utils

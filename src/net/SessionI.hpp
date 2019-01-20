#pragma once

#include "log/Logger.hpp"
#include <boost/asio.hpp>
#include <thread>
#include <vector>

namespace utils {
namespace config {
class ServerConfig;
} // namespace config
} // namespace utils

namespace utils {
namespace net {

class SessionI {
public:
  SessionI() {}

  virtual ~SessionI() {}

  virtual void send(std::shared_ptr<std::string> ss) = 0;

  virtual void send(const std::string& ss) = 0;

protected:
};

} // namespace net
} // namespace utils

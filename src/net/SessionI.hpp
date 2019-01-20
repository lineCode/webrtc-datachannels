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
namespace algo {
class DispatchQueue;
} // namespace algo
} // namespace utils

namespace utils {
namespace net {

class SessionI {
public:
  SessionI(/*const std::string& id*/);

  virtual ~SessionI() {}

  virtual void send(std::shared_ptr<std::string> ss) = 0;

  virtual void send(const std::string& ss) = 0;

protected:
  // const std::string id_;
};

} // namespace net
} // namespace utils

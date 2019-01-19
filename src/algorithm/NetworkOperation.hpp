#pragma once

#include "log/Logger.hpp"
#include <cstdint>
#include <string>

namespace utils {
namespace net {

class Opcodes {
public:
  enum class WS_OPCODE : uint32_t { PING = 0, CANDIDATE = 1, OFFER = 2, ANSWER = 3, TOTAL = 4 };

  static std::string opcodeToStr(const WS_OPCODE& code) {
    switch (code) {
    case WS_OPCODE::PING:
      return std::string{"ping"};
    case WS_OPCODE::CANDIDATE:
      return std::string{"candidate"};
    case WS_OPCODE::OFFER:
      return std::string{"offer"};
    case WS_OPCODE::ANSWER:
      return std::string{"answer"};
    default: {
      LOG(WARNING) << "INVALID opcodeToStr!";
      return "";
    }
    }
  }

  // TODO
  /*enum class WRTC_OPCODE : uint32_t { PING = 0, CANDIDATE = 1, OFFER = 2, ANSWER = 3 };

    static std::string opcodeToStr(const WRTC_OPCODE& code) {
      return std::to_string(static_cast<uint32_t>(code));
    }*/

  static WS_OPCODE opcodeFromStr(const std::string& str) {
    if (str == "ping")
      return WS_OPCODE::PING;

    if (str == "candidate")
      return WS_OPCODE::CANDIDATE;

    if (str == "offer")
      return WS_OPCODE::OFFER;

    if (str == "answer")
      return WS_OPCODE::ANSWER;

    LOG(WARNING) << "INVALID opcodeFromStr!";
    return WS_OPCODE::TOTAL;
    // return std::to_string(static_cast<uint32_t>(code));
  }
};

using WS_OPCODE = Opcodes::WS_OPCODE;
// using WRTC_OPCODE = Opcodes::WRTC_OPCODE;

template <typename T> struct NetworkOperation {
  NetworkOperation(const T& operationCode, const std::string& operationName)
      : operationCode_(operationCode), operationCodeStr_(Opcodes::opcodeToStr(operationCode)),
        operationName_(operationName) {}

  NetworkOperation(const T& operationCode)
      : operationCode_(operationCode), operationCodeStr_(Opcodes::opcodeToStr(operationCode)),
        operationName_("") {}

  bool operator<(const NetworkOperation& rhs) const { return operationCode_ < rhs.operationCode_; }

  const T operationCode_;

  const std::string operationCodeStr_;

  /**
   * operationName usefull for logging
   * NOTE: operationName may be empty
   **/
  const std::string operationName_;
};

} // namespace net
} // namespace utils

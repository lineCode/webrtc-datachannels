#pragma once

#include "log/Logger.hpp"
#include <cstdint>
#include <string>

namespace utils {
namespace net {

class Opcodes {
public:
  enum class WS_OPCODE : uint32_t { PING = 0, CANDIDATE = 1, OFFER = 2, ANSWER = 3 };

  static std::string opcodeToStr(const WS_OPCODE& code) {
    switch (code) {
    case WS_OPCODE::PING:
      return "0";
    case WS_OPCODE::CANDIDATE:
      return "1";
    case WS_OPCODE::OFFER:
      return "2";
    case WS_OPCODE::ANSWER:
      return "3";
      /*default: {
        LOG(WARNING) << "INVALID opcodeToStr!";
        return "";
      }*/
    }

    LOG(WARNING) << "INVALID opcodeToStr!";
    return "";
    // return std::to_string(static_cast<uint32_t>(code));
  }

  // TODO
  /*enum class WRTC_OPCODE : uint32_t { PING = 0, CANDIDATE = 1, OFFER = 2, ANSWER = 3 };

  static std::string opcodeToStr(const WRTC_OPCODE& code) {
    return std::to_string(static_cast<uint32_t>(code));
  }*/
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

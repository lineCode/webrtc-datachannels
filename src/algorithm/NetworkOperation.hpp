#pragma once

#include "log/Logger.hpp"
#include <string>

namespace utils {
namespace algo {

class Opcodes {
public:
  enum class WS_OPCODE : uint32_t { PING = 0, CANDIDATE = 1, OFFER = 2, ANSWER = 3, TOTAL = 4 };

  enum class WRTC_OPCODE : uint32_t { PING = 0, SERVER_TIME = 1, TOTAL = 4 };

  // static std::string opcodeToDescrStr(const WS_OPCODE& code);

  static std::string opcodeToStr(const WS_OPCODE& code);

  static WS_OPCODE wsOpcodeFromStr(const std::string& str);

  // static WS_OPCODE wsOpcodeFromDescrStr(const std::string& str);

  // static std::string opcodeToDescrStr(const WRTC_OPCODE& code);

  static std::string opcodeToStr(const WRTC_OPCODE& code);

  static WRTC_OPCODE wrtcOpcodeFromStr(const std::string& str);

  // static WRTC_OPCODE wrtcOpcodeFromDescrStr(const std::string& str);
};

using WS_OPCODE = Opcodes::WS_OPCODE;
using WRTC_OPCODE = Opcodes::WRTC_OPCODE;

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

} // namespace algo
} // namespace utils

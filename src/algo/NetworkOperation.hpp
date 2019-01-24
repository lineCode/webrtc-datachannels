#pragma once

#include "log/Logger.hpp"
#include <enum.h>
#include <string>

namespace gloer {
namespace algo {

BETTER_ENUM(WS_OPCODE_ENUM, uint32_t, PING, CANDIDATE, OFFER, ANSWER, TOTAL)
BETTER_ENUM(WRTC_OPCODE_ENUM, uint32_t, PING, SERVER_TIME, KEEPALIVE, TOTAL)

class Opcodes {
public:
  using WS_OPCODE = WS_OPCODE_ENUM;
  using WRTC_OPCODE = WRTC_OPCODE_ENUM;

  static std::string opcodeToDescrStr(const WS_OPCODE& code);

  static std::string opcodeToStr(const WS_OPCODE& code);

  static WS_OPCODE wsOpcodeFromStr(const std::string& str);

  static WS_OPCODE wsOpcodeFromDescrStr(const std::string& str);

  static std::string opcodeToDescrStr(const WRTC_OPCODE& code);

  static std::string opcodeToStr(const WRTC_OPCODE& code);

  static WRTC_OPCODE wrtcOpcodeFromStr(const std::string& str);

  static WRTC_OPCODE wrtcOpcodeFromDescrStr(const std::string& str);
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
} // namespace gloer

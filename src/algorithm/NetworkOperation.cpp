#include "algorithm/NetworkOperation.hpp" // IWYU pragma: associated
#include <cinttypes>
#include <cstdint>

namespace utils {
namespace algo {

// TODO: use reflection: WS_OPCODE::PING -> "PING"
/*std::string Opcodes::opcodeToDescrStr(const WS_OPCODE& code) {
  switch (code) {
  case WS_OPCODE::PING:
    return "PING";
  case WS_OPCODE::CANDIDATE:
    return "CANDIDATE";
  case WS_OPCODE::OFFER:
    return "OFFER";
  case WS_OPCODE::ANSWER:
    return "ANSWER";
  default: {
    LOG(WARNING) << "INVALID opcodeToDescrStr!";
    return "";
  }
  }
}*/

std::string Opcodes::opcodeToStr(const WS_OPCODE& code) {
  return std::to_string(static_cast<uint32_t>(code));
}

WS_OPCODE Opcodes::wsOpcodeFromStr(const std::string& str) {
  // TODO str -> uint32_t: to separate UTILS file
  // see https://stackoverflow.com/a/5745454/10904212
  uint32_t type; // NOTE: on change: don`t forget about UINT32_FIELD_MAX_LEN
  sscanf(str.c_str(), "%" SCNu32, &type);
  return static_cast<WS_OPCODE>(type);
}

// TODO: use reflection: "PING" -> WS_OPCODE::PING
/*WS_OPCODE Opcodes::wsOpcodeFromDescrStr(const std::string& str) {
  if (str == "PING")
    return WS_OPCODE::PING;

  if (str == "CANDIDATE")
    return WS_OPCODE::CANDIDATE;

  if (str == "OFFER")
    return WS_OPCODE::OFFER;

  if (str == "ANSWER")
    return WS_OPCODE::ANSWER;

  LOG(WARNING) << "INVALID wsOpcodeFromDescrStr!";
  return WS_OPCODE::TOTAL;
}*/

/*std::string Opcodes::opcodeToDescrStr(const WRTC_OPCODE& code) {
  switch (code) {
  case WRTC_OPCODE::PING:
    return "SERVER_TIME";
  case WRTC_OPCODE::SERVER_TIME:
  default: {
    LOG(WARNING) << "INVALID opcodeToDescrStr!";
    return "";
  }
  }
}*/

std::string Opcodes::opcodeToStr(const WRTC_OPCODE& code) {
  return std::to_string(static_cast<uint32_t>(code));
}

WRTC_OPCODE Opcodes::wrtcOpcodeFromStr(const std::string& str) {
  // TODO str -> uint32_t: to separate UTILS file
  // see https://stackoverflow.com/a/5745454/10904212
  uint32_t type; // NOTE: on change: don`t forget about UINT32_FIELD_MAX_LEN
  sscanf(str.c_str(), "%" SCNu32, &type);
  return static_cast<WRTC_OPCODE>(type);
}

/*WRTC_OPCODE Opcodes::wrtcOpcodeFromDescrStr(const std::string& str) {
  if (str == "PING")
    return WRTC_OPCODE::PING;

  if (str == "SERVER_TIME")
    return WRTC_OPCODE::SERVER_TIME;

  LOG(WARNING) << "INVALID wrtcOpcodeFromStr!";
  return WRTC_OPCODE::TOTAL;
}*/

} // namespace algo
} // namespace utils

#include "algo/NetworkOperation.hpp" // IWYU pragma: associated
#include <cinttypes>
#include <cstdint>

namespace gloer {
namespace algo {

std::string Opcodes::opcodeToDescrStr(const WS_OPCODE& code) { return code._to_string(); }

std::string Opcodes::opcodeToStr(const WS_OPCODE& code) {
  return std::to_string(static_cast<uint32_t>(code));
}

WS_OPCODE Opcodes::wsOpcodeFromStr(const std::string& str) {
  // TODO str -> uint32_t: to separate UTILS file
  // @see stackoverflow.com/a/5745454/10904212
  uint32_t type; // NOTE: on change: don`t forget about UINT32_FIELD_MAX_LEN
  sscanf(str.c_str(), "%" SCNu32, &type);
  return WS_OPCODE::_from_integral(type);
}

WS_OPCODE Opcodes::wsOpcodeFromDescrStr(const std::string& str) {
  return WS_OPCODE::_from_string(str.c_str());
}

std::string Opcodes::opcodeToDescrStr(const WRTC_OPCODE& code) { return code._to_string(); }

std::string Opcodes::opcodeToStr(const WRTC_OPCODE& code) {
  return std::to_string(static_cast<uint32_t>(code));
}

WRTC_OPCODE Opcodes::wrtcOpcodeFromStr(const std::string& str) {
  // TODO str -> uint32_t: to separate UTILS file
  // @see stackoverflow.com/a/5745454/10904212
  uint32_t type; // NOTE: on change: don`t forget about UINT32_FIELD_MAX_LEN
  sscanf(str.c_str(), "%" SCNu32, &type);
  return WRTC_OPCODE::_from_integral(type);
}

WRTC_OPCODE Opcodes::wrtcOpcodeFromDescrStr(const std::string& str) {
  return WRTC_OPCODE::_from_string(str.c_str());
}

} // namespace algo
} // namespace gloer

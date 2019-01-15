#include "lua/LuaScript.hpp" // IWYU pragma: associated
#include "filesystem/path.hpp"
#include <string>

namespace utils {
namespace lua {

sol::state* LuaScript::loadScriptFile(const fs::path& path) {
  std::string scriptContents = utils::filesystem::getFileContents(path);
  // TODO: compare with lua.script_file("config.lua");
  lua.script(scriptContents);
  return &lua;
}

} // namespace lua
} // namespace utils

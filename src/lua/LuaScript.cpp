#include "lua/LuaScript.hpp" // IWYU pragma: associated
#include "storage/path.hpp"
#include <string>

namespace fs = std::filesystem; // from <filesystem>

namespace gloer {
namespace lua {

sol::state* LuaScript::loadScriptFile(const ::fs::path& path) {
  std::string scriptContents = gloer::storage::getFileContents(path);
  // TODO: compare with lua.script_file("config.lua");
  lua_.script(scriptContents);
  return &lua_;
}

} // namespace lua
} // namespace gloer

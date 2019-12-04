#if 0

#include "lua/LuaScript.hpp" // IWYU pragma: associated
#include "storage/path.hpp"
#include <string>

#ifndef __has_include
  static_assert(false, "__has_include not supported");
#else
#  if __has_include(<filesystem>)
#    include <filesystem>
     namespace fs = std::filesystem;
#  elif __has_include(<experimental/filesystem>)
#    include <experimental/filesystem>
     namespace fs = std::experimental::filesystem;
#  elif __has_include(<boost/filesystem.hpp>)
#    include <boost/filesystem.hpp>
     namespace fs = boost::filesystem;
#  endif
#endif

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

#endif // 0

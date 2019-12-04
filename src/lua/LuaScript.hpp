#pragma once

#if 0

#include <sol/sol.hpp> // IWYU pragma: export

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

class LuaScript {
public:
  LuaScript() : lua_(sol::state()){};
  sol::state* loadScriptFile(const fs::path& path);

private:
  sol::state lua_;
};

} // namespace lua
} // namespace gloer

#endif // 0

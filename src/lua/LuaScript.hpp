#pragma once

#include <filesystem>
#include <sol3/sol.hpp> // IWYU pragma: export

namespace utils {
namespace lua {

class LuaScript {
public:
  LuaScript() : lua_(sol::state()){};
  sol::state* loadScriptFile(const std::filesystem::path& path);

private:
  sol::state lua_;
};

} // namespace lua
} // namespace utils

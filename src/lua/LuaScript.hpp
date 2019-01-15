#pragma once

#include <filesystem>
#include <sol3/sol.hpp> // IWYU pragma: export

namespace utils {
namespace lua {

class LuaScript {
public:
  LuaScript() : lua(sol::state()){};
  sol::state* loadScriptFile(const std::filesystem::path& path);

private:
  sol::state lua;
};

} // namespace lua
} // namespace utils

#pragma once

#include <filesystem>
#include <sol3/sol.hpp> // IWYU pragma: export

namespace fs = std::filesystem;

namespace utils {
namespace lua {

class LuaScript {
public:
  LuaScript() : lua(sol::state()){};
  sol::state* loadScriptFile(const fs::path& path);

private:
  sol::state lua;
};

} // namespace lua
} // namespace utils

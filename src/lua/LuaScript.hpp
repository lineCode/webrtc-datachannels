#pragma once

#include <filesystem>
#include <sol/sol.hpp> // IWYU pragma: export

namespace gloer {
namespace lua {

class LuaScript {
public:
  LuaScript() : lua_(sol::state()){};
  sol::state* loadScriptFile(const std::filesystem::path& path);

private:
  sol::state lua_;
};

} // namespace lua
} // namespace gloer

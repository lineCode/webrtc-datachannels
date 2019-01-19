#include "algorithm/StringUtils.hpp" // IWYU pragma: associated

namespace utils {
namespace string {

std::string trim_whitespace(std::string const& str) {
  const std::string whitespace = " \t\f\v\n\r";
  auto start = str.find_first_not_of(whitespace);
  auto end = str.find_last_not_of(whitespace);
  if (start == std::string::npos || end == std::string::npos) {
    return std::string();
  }
  return str.substr(start, end + 1);
}

} // namespace string
} // namespace utils

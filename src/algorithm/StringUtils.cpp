#include "algorithm/StringUtils.hpp" // IWYU pragma: associated
#include <boost/lexical_cast.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace utils {
namespace algo {

std::string trim_whitespace(std::string const& str) {
  const std::string whitespace = " \t\f\v\n\r";
  auto start = str.find_first_not_of(whitespace);
  auto end = str.find_last_not_of(whitespace);
  if (start == std::string::npos || end == std::string::npos) {
    return std::string();
  }
  return str.substr(start, end + 1);
}

boost::uuids::uuid genBoostGuid() {
  // return sm->maxSessionId() + 1; // TODO: collision
  boost::uuids::random_generator generator;
  return generator();
}

std::string genGuid() { return boost::lexical_cast<std::string>(genBoostGuid()); }

} // namespace algo
} // namespace utils

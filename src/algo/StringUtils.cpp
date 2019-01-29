#include "algo/StringUtils.hpp" // IWYU pragma: associated
#include <boost/lexical_cast.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace gloer {
namespace algo {

boost::uuids::uuid genBoostGuid() {
  // return sm->maxSessionId() + 1; // TODO: collision
  boost::uuids::random_generator generator;
  return generator();
}

std::string genGuid() { return boost::lexical_cast<std::string>(genBoostGuid()); }

} // namespace algo
} // namespace gloer

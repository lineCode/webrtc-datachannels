#include "net/SessionPair.hpp" // IWYU pragma: associated

namespace gloer {
namespace net {

SessionPair::SessionPair(const ws::SessionGUID& id) : SessionBase<ws::SessionGUID>(id) {}

} // namespace net
} // namespace gloer

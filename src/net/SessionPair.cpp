#include "net/SessionPair.hpp"
#include "net/SessionBase.hpp"
#include "net/ws/SessionGUID.hpp"

namespace gloer {
namespace net {

SessionPair::SessionPair(const ws::SessionGUID& id) : SessionBase<ws::SessionGUID>(id) {}

} // namespace net
} // namespace gloer

#pragma once

#include <map>

namespace gloer {
namespace algo {

template <typename opType, typename cbType> class CallbackManager {
public:
  CallbackManager() {}

  virtual ~CallbackManager(){};

  virtual std::map<opType, cbType> getCallbacks() const = 0;

  virtual void addCallback(const opType& op, const cbType& cb) = 0;

protected:
  std::map<opType, cbType> operationCallbacks_;
};

} // namespace algo
} // namespace utils

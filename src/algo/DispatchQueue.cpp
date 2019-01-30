#include "algo/DispatchQueue.hpp" // IWYU pragma: associated
#include "log/Logger.hpp"
#include <algorithm>
#include <iostream>

namespace gloer {
namespace algo {

DispatchQueue::DispatchQueue(const std::string& name, const size_t thread_cnt) : name_(name) {
  LOG(INFO) << "Creating dispatch queue: " << name.c_str();
  LOG(INFO) << "Dispatch threads: " << thread_cnt;
}

DispatchQueue::~DispatchQueue() {
  LOG(INFO) << "Destructor: Destroying dispatch threads...";

  quit_ = true;
}

void DispatchQueue::dispatch(dispatch_callback op) {
  if (callbacksQueue_.isFull()) {
    LOG(WARNING) << "DispatchQueue::dispatch: full queue: " << name_;
    return;
  }

  // Emplace a value at the end of the queue, returns false if the queue was full.
  if (!callbacksQueue_.write(op)) {
    LOG(WARNING) << "DispatchQueue::dispatch: full queue: " << name_;
    return;
  }
}

/*void DispatchQueue::dispatch(dispatch_callback&& op) {
  if (callbacksQueue_.isFull()) {
    LOG(WARNING) << "DispatchQueue::dispatch: full queue: " << name_;
    return;
  }

  callbacksQueue_.write(std::move(op));
}*/

void DispatchQueue::DispatchQueued(void) {
  if (!callbacksQueue_.isEmpty()) {
    /*
Returns the number of entries in the queue. Because of the way we coordinate threads, this guess
could be slightly wrong when called by the producer/consumer thread, and it could be wildly
inaccurate if called from any other threads. Hence, only call from producer/consumer threads!
     */
    // LOG(WARNING) << "callbacksQueue_.sizeGuess = " << callbacksQueue_.sizeGuess();
  }

  do {
    if (!quit_ && !callbacksQueue_.isEmpty()) {
      dispatch_callback* dispatchCallback;

      dispatchCallback = callbacksQueue_.frontPtr();

      if (!dispatchCallback) {
        LOG(WARNING) << "DispatchQueue dispatch_thread_handler: invalid dispatchCallback from "
                     << name_;
        continue;
      }

      (*dispatchCallback)();

      callbacksQueue_.popFront();
    }
  } while (!callbacksQueue_.isEmpty() && !quit_);
}

} // namespace algo
} // namespace gloer

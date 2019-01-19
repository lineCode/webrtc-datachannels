#include "algorithm/DispatchQueue.hpp" // IWYU pragma: associated
#include "log/Logger.hpp"
#include <algorithm>
#include <iostream>

namespace utils {
namespace algo {

DispatchQueue::DispatchQueue(const std::string& name, const size_t thread_cnt)
    : name_(name), threads_(thread_cnt) {
  LOG(INFO) << "Creating dispatch queue: " << name.c_str();
  LOG(INFO) << "Dispatch threads: " << thread_cnt;

  // NOTE: threads_.size() may be 0 -> use parent thread
  for (size_t i = 0; i < threads_.size(); i++) {
    threads_[i] = std::thread(&DispatchQueue::dispatch_loop, this);
  }
}

DispatchQueue::~DispatchQueue() {
  LOG(INFO) << "Destructor: Destroying dispatch threads...";

  // Signal to dispatch threads that it's time to wrap up
  std::unique_lock<std::mutex> lock(lock_);
  quit_ = true;
  lock.unlock();
  cv_.notify_all();

  // NOTE: threads_.size() may be 0 -> use parent thread
  // Wait for threads to finish before we exit
  for (size_t i = 0; i < threads_.size(); i++) {
    if (threads_[i].joinable()) {
      LOG(INFO) << "Destructor: Joining thread " << i << " until completion";
      threads_[i].join();
    }
  }
}

void DispatchQueue::dispatch(const dispatch_callback& op) {
  std::unique_lock<std::mutex> lock(lock_);
  callbacksQueue_.push(op);

  // Manual unlocking is done before notifying, to avoid waking up
  // the waiting thread only to block again (see notify_one for details)
  lock.unlock();
  cv_.notify_all();
}

void DispatchQueue::dispatch(dispatch_callback&& op) {
  std::unique_lock<std::mutex> lock(lock_);
  callbacksQueue_.push(std::move(op));

  // Manual unlocking is done before notifying, to avoid waking up
  // the waiting thread only to block again (see notify_one for details)
  lock.unlock();
  cv_.notify_all();
}

// NOTE: Runs in loop and blocks execution (if runs in same thread)
void DispatchQueue::dispatch_loop(void) {
  std::unique_lock<std::mutex> lock(lock_);

  do {
    // Wait until we have data or a quit signal
    cv_.wait(lock, [this] { return (callbacksQueue_.size() || quit_); });

    // after wait, we own the lock
    if (!quit_ && callbacksQueue_.size()) {
      auto dispatchCallback = std::move(callbacksQueue_.front());
      callbacksQueue_.pop();

      // unlock now that we're done messing with the queue
      lock.unlock();

      LOG(INFO) << "DispatchQueue dispatch_thread_handler for " << name_;
      dispatchCallback();

      lock.lock();
    }
  } while (!quit_);

  lock.unlock();
}

void DispatchQueue::DispatchQueued(void) {
  std::unique_lock<std::mutex> lock(lock_);

  do {
    if (!quit_ && callbacksQueue_.size()) {
      auto dispatchCallback = std::move(callbacksQueue_.front());
      callbacksQueue_.pop();

      // unlock now that we're done messing with the queue
      lock.unlock();

      LOG(INFO) << "DispatchQueue dispatch_thread_handler for " << name_;
      dispatchCallback();

      lock.lock();
    }
  } while (callbacksQueue_.size() && !quit_);

  lock.unlock();
}

} // namespace algo
} // namespace utils

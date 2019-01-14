#include "dispatch_queue.hpp"
#include <algorithm>
#include <iostream>

dispatch_queue::dispatch_queue(const std::string& name, size_t thread_cnt)
    : name_(name), threads_(thread_cnt) {
  printf("Creating dispatch queue: %s\n", name.c_str());
  printf("Dispatch threads: %zu\n", thread_cnt);

  // NOTE: threads_.size() may be 0 -> use parent thread
  for (size_t i = 0; i < threads_.size(); i++) {
    threads_[i] = std::thread(&dispatch_queue::dispatch_loop, this);
  }
}

dispatch_queue::~dispatch_queue() {
  printf("Destructor: Destroying dispatch threads...\n");

  // Signal to dispatch threads that it's time to wrap up
  std::unique_lock<std::mutex> lock(lock_);
  quit_ = true;
  lock.unlock();
  cv_.notify_all();

  // NOTE: threads_.size() may be 0 -> use parent thread
  // Wait for threads to finish before we exit
  for (size_t i = 0; i < threads_.size(); i++) {
    if (threads_[i].joinable()) {
      printf("Destructor: Joining thread %zu until completion\n", i);
      threads_[i].join();
    }
  }
}

void dispatch_queue::dispatch(const dispatch_callback& op) {
  std::unique_lock<std::mutex> lock(lock_);
  callbacksQueue_.push(op);

  // Manual unlocking is done before notifying, to avoid waking up
  // the waiting thread only to block again (see notify_one for details)
  lock.unlock();
  cv_.notify_all();
}

void dispatch_queue::dispatch(dispatch_callback&& op) {
  std::unique_lock<std::mutex> lock(lock_);
  callbacksQueue_.push(std::move(op));

  // Manual unlocking is done before notifying, to avoid waking up
  // the waiting thread only to block again (see notify_one for details)
  lock.unlock();
  cv_.notify_all();
}

// NOTE: Runs in loop and blocks execution (if runs in same thread)
void dispatch_queue::dispatch_loop(void) {
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

      std::cout << "dispatch_queue dispatch_thread_handler for " << name_
                << std::endl;
      dispatchCallback();

      lock.lock();
    }
  } while (!quit_);

  lock.unlock();
}

void dispatch_queue::dispatch_queued(void) {
  std::unique_lock<std::mutex> lock(lock_);

  do {
    if (!quit_ && callbacksQueue_.size()) {
      auto dispatchCallback = std::move(callbacksQueue_.front());
      callbacksQueue_.pop();

      // unlock now that we're done messing with the queue
      lock.unlock();

      std::cout << "dispatch_queue dispatch_thread_handler for " << name_
                << std::endl;
      dispatchCallback();

      lock.lock();
    }
  } while (callbacksQueue_.size() && !quit_);

  lock.unlock();
}

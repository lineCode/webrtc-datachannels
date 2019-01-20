#pragma once

#include <condition_variable>
#include <cstdio>
#include <folly/ProducerConsumerQueue.h>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace utils {
namespace algo {

// NOTE: ProducerConsumerQueue must be created with a fixed maximum size
// We use Queue per connection, so maxQueueElems is for 1 client
constexpr size_t maxQueueElems = 512;

/*
 * DispatchQueue: Based on
 * https://embeddedartistry.com/blog/2017/2/1/c11-implementing-a-dispatch-queue-using-stdfunction
 * https://github.com/seanlaguna/contentious/blob/master/contentious/threadpool.cc
 **/
class DispatchQueue {
public:
  typedef std::function<void(void)> dispatch_callback;

  DispatchQueue(const std::string& name, const size_t thread_cnt = 1);
  ~DispatchQueue();

  // dispatch and copy
  void dispatch(dispatch_callback op);

  // dispatch and move
  // void dispatch(dispatch_callback&& op);

  // Deleted operations
  // TODO
  DispatchQueue(const DispatchQueue& rhs) = delete;
  DispatchQueue& operator=(const DispatchQueue& rhs) = delete;
  DispatchQueue(DispatchQueue&& rhs) = delete;
  DispatchQueue& operator=(DispatchQueue&& rhs) = delete;

  void DispatchQueued(void);

  bool isEmpty() const { return callbacksQueue_.isEmpty(); }

  bool isFull() const { return callbacksQueue_.isFull(); }

private:
  std::string name_;

  // std::mutex lock_; // folly::ProducerConsumerQueue is lock-free

  /*
   * ProducerConsumerQueue is a one producer and one consumer queue
   * without locks.
   */
  folly::ProducerConsumerQueue<dispatch_callback> callbacksQueue_{maxQueueElems};

  // std::condition_variable cv_;
  bool quit_ = false;
};

} // namespace algo
} // namespace utils

#pragma once

#include <condition_variable>
#include <cstdio>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace utils {
namespace algo {

/*
 * DispatchQueue: Based on
 * https://embeddedartistry.com/blog/2017/2/1/c11-implementing-a-dispatch-queue-using-stdfunction
 **/
class DispatchQueue {
  typedef std::function<void(void)> dispatch_callback;

public:
  DispatchQueue(const std::string& name, size_t thread_cnt = 1);
  ~DispatchQueue();

  // dispatch and copy
  void dispatch(const dispatch_callback& op);
  // dispatch and move
  void dispatch(dispatch_callback&& op);

  // Deleted operations
  // TODO
  DispatchQueue(const DispatchQueue& rhs) = delete;
  DispatchQueue& operator=(const DispatchQueue& rhs) = delete;
  DispatchQueue(DispatchQueue&& rhs) = delete;
  DispatchQueue& operator=(DispatchQueue&& rhs) = delete;

  // private:
  std::string name_;
  std::mutex lock_;
  std::vector<std::thread> threads_;
  std::queue<dispatch_callback> callbacksQueue_;
  std::condition_variable cv_;
  bool quit_ = false;

  void dispatch_loop(void);

  void DispatchQueued(void);

  bool empty() {
    std::scoped_lock<std::mutex> lock(lock_);
    return threads_.empty();
  }
};

} // namespace algo
} // namespace utils

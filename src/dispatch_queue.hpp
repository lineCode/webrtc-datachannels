#pragma once

/*
 * dispatch_queue: Based on
 *https://embeddedartistry.com/blog/2017/2/1/c11-implementing-a-dispatch-queue-using-stdfunction
 **/

#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

class dispatch_queue {
  typedef std::function<void(void)> dispatch_callback;

public:
  // dispatch_queue(const std::string& name);
  dispatch_queue(const std::string& name, size_t thread_cnt = 1);
  ~dispatch_queue();

  // dispatch and copy
  void dispatch(const dispatch_callback& op);
  // dispatch and move
  void dispatch(dispatch_callback&& op);

  // Deleted operations
  // TODO
  dispatch_queue(const dispatch_queue& rhs) = delete;
  dispatch_queue& operator=(const dispatch_queue& rhs) = delete;
  dispatch_queue(dispatch_queue&& rhs) = delete;
  dispatch_queue& operator=(dispatch_queue&& rhs) = delete;

  // private:
  std::string name_;
  std::mutex lock_;
  std::vector<std::thread> threads_;
  std::queue<dispatch_callback> callbacksQueue_;
  std::condition_variable cv_;
  bool quit_ = false;

  void dispatch_loop(void);

  void dispatch_queued(void);

  bool empty() {
    std::scoped_lock<std::mutex> lock(lock_);
    return threads_.empty();
  }
};

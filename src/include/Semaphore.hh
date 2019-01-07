#ifndef __SEMAPHORE_HH__
#define __SEMAPHORE_HH__

#include <condition_variable>
#include <mutex>

namespace m3
{

// code from http://blog.csdn.net/dongzhiliu/article/details/76083905
class Semaphore {
public:
  Semaphore(long count = 0)
    : mutex_(), cv_(), count_(count) {
  }

  void release() {
    std::unique_lock<std::mutex> lock(mutex_);
    ++count_;
    cv_.notify_one();
  }

  void issue() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [=] { return count_ > 0; });
    --count_;
  }

private:
  std::mutex mutex_;
  std::condition_variable cv_;
  long count_;
};

}

#endif

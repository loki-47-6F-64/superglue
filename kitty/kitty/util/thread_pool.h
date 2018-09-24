#ifndef KITTY_THREAD_POOL_H
#define KITTY_THREAD_POOL_H

#include <kitty/util/task_pool.h>
#include <kitty/util/thread_t.h>

namespace util {
/*
 * Allow threads to execute unhindered
 * while keeping full controll over the threads.
 */
template<class Thread = thread_t>
class ThreadPoolWith : public TaskPool {
public:
  typedef TaskPool::__task __task;
  
private:
  std::vector<Thread> _thread;

  std::condition_variable _cv;
  std::mutex _lock;
  
  std::atomic<bool> _continue;
public:

  ThreadPoolWith(int threads) : _thread(threads), _continue(true) {
    for (auto & t : _thread) {
      t = Thread(&ThreadPoolWith::_main, this);
    }
  }

  ~ThreadPoolWith() {
    join();
  }

  template<class Function, class... Args>
  auto push(Function && newTask, Args &&... args) {
    auto future = TaskPool::push(std::forward<Function>(newTask), std::forward<Args>(args)...);
    
    _cv.notify_one();
    return future;
  }

  template<class Function, class X, class Y, class... Args>
  auto pushDelayed(Function &&newTask, std::chrono::duration<X, Y> duration, Args &&... args) {
    auto future = TaskPool::pushDelayed(std::forward<Function>(newTask), duration, std::forward<Args>(args)...);

    // Update all timers for wait_until
    _cv.notify_all();
    return future;
  }
  
  void join() {
    if (!_continue.exchange(false)) return;
    
    _cv.notify_all();
    for (auto & t : _thread) {
      t.join();
    }
  }

public:

  void _main() {
    while (_continue.load()) {
      if(auto task = this->pop()) {
        (*task)->run();
      }
      else {
        std::unique_lock<std::mutex> uniq_lock(_lock);
        _cv.wait_until(uniq_lock, next());
      }
    }

    // Execute remaining tasks
    while(auto task = this->pop()) {
      (*task)->run();
    }
  }
};

typedef ThreadPoolWith<thread_t> ThreadPool;
}
#endif

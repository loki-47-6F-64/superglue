#ifndef KITTY_TASK_POOL_H
#define KITTY_TASK_POOL_H

#include <deque>
#include <vector>
#include <future>
#include <chrono>
#include <utility>

#include <kitty/util/optional.h>
#include <kitty/util/utility.h>
#include <kitty/util/thread_t.h>
namespace util {

class TaskPool {
public:
  typedef std::unique_ptr<_ImplBase> __task;
  
  typedef std::chrono::steady_clock::time_point __time_point;
private:
  std::deque<__task> _tasks;
  std::vector<std::pair<__time_point, __task>> _timer_tasks; 
  std::mutex _task_mutex;

public:
  template<class Function, class... Args>
  auto push(Function && newTask, Args &&... args) {
    typedef decltype(newTask(std::forward<Args>(args)...)) __return;
    typedef std::packaged_task<__return()> task_t;
    
    task_t task(std::bind(
      std::forward<Function>(newTask),
      std::forward<Args>(args)...
    ));
    
    auto future = task.get_future();
    
    std::lock_guard<std::mutex> lg(_task_mutex);
    _tasks.emplace_back(toRunnable(std::move(task)));
    
    return future;
  }

  template<class Function, class X, class Y, class... Args>
  auto pushTimed(Function &&newTask, std::chrono::duration<X, Y> duration, Args &&... args) {
    typedef decltype(newTask(std::forward<Args>(args)...)) __return;
    typedef std::packaged_task<__return()> task_t;
    
    __time_point time_point = std::chrono::steady_clock::now() + duration;

    task_t task(std::bind(
      std::forward<Function>(newTask),
      std::forward<Args>(args)...
    ));

    auto future = task.get_future();
    
    std::lock_guard<std::mutex> lg(_task_mutex);
    
    auto it = _timer_tasks.cbegin();
    for(; it < _timer_tasks.cend(); ++it) {
      if(std::get<0>(*it) < time_point) {
        break;
      }
    }
    _timer_tasks.emplace(it, time_point, toRunnable(std::move(task)));
    
    return future;
  }
  
  util::Optional<__task> pop() {
    std::lock_guard<std::mutex> lg(_task_mutex);
    
    if(!_tasks.empty()) {
      __task task = std::move(_tasks.front());
      _tasks.pop_front();
      return std::move(task);
    }
    
    if(!_timer_tasks.empty() && std::get<0>(_timer_tasks.back()) <= std::chrono::steady_clock::now()) {
      __task task = std::move(std::get<1>(_timer_tasks.back()));
      _timer_tasks.pop_back();
      
      return std::move(task);
    }
    
    return {};
  }

  __time_point next() {
    std::lock_guard<std::mutex> lg(_task_mutex);

    if(_timer_tasks.empty()) {
      return std::chrono::time_point<std::chrono::steady_clock>::max();
    }

    return std::get<0>(_timer_tasks.back());
  }
private:
  
  template<class Function>
  std::unique_ptr<_ImplBase> toRunnable(Function &&f) {
    return util::mk_uniq<_Impl<Function>>(std::move(f));
  }
};
}
#endif

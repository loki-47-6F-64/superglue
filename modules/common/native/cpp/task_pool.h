//
// Created by loki on 25-9-18.
//

#ifndef SUPERGLUE_TASK_POOL_H
#define SUPERGLUE_TASK_POOL_H

#include <kitty/util/task_pool.h>

#include "config.hpp"
class TaskPool : public util::TaskPool {
  std::atomic_bool _continue;
  std::atomic_int _task_running;
public:
  TaskPool() : _continue(false), _task_running(false) {}

  template<class Function, class... Args>
  auto push(Function && newTask, Args &&... args) {
    typedef decltype(newTask(std::forward<Args>(args)...)) __return;

    if(!_continue.load()) {
      return std::future<__return> {};
    }

    return util::TaskPool::push(std::forward<Function>(newTask), std::forward<Args>(args)...);
  }

  template<class Function, class X, class Y, class... Args>
  auto pushDelayed(Function &&newTask, std::chrono::duration<X, Y> duration, Args &&... args) {
    typedef decltype(newTask(std::forward<Args>(args)...)) __return;

    if(!_continue.load()) {
      std::future<__return> future;
      return timer_task_t<__return> { nullptr, future };
    }

    return util::TaskPool::pushDelayed(std::forward<Function>(newTask), duration, std::forward<Args>(args)...);
  }

  void run(const __task &task) {
    ++_task_running;

    if(_continue.load()) {
      auto _begin = std::chrono::steady_clock::now();
      task->run();
      auto milli = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - _begin);

      if(milli.count() > 10) {
        logManager->log(gen::LogSeverity::WARN, "duration of task => [" + std::to_string(milli.count()) + "] milliseconds");
      }
    }

    --_task_running;
  }

  // Remove pending to tasks and wait for current task to be completed.
  void clear() {
    if(_continue.exchange(false)) {
      std::lock_guard<std::mutex> lg(_task_mutex);

      _timer_tasks.clear();
      _tasks.clear();
    }

    while(_task_running.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  void start() {
    _continue.store(true);
  }
};

#endif //SUPERGLUE_TASK_POOL_H

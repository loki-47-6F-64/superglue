#pragma once

#include "config.hpp"
#include "thread_callback.hpp"

#include <system_error>
#include <functional>
#include <thread>

template<class Function>
class _Impl : public gen::ThreadCallback {
  Function _func;
  
public:
  
  _Impl(std::thread::native_handle_type &id, Function&& f) : _func(std::forward<Function>(f)) {
    id = pthread_self();
  }
  
  void run() {
    _func();
  }
};

class thread_t {
  std::thread::native_handle_type _id;
public:
  
  thread_t() : _id(0) { }
  
  thread_t(thread_t &&other) {
    std::swap(this->_id, other._id);
  }
  
  void operator=(thread_t &&other) {
    std::swap(this->_id, other._id);
  }
  
  template<class Function, class... Args>
  explicit thread_t(Function&& f, Args&&... args) : _id(0) {
    _startThread(std::bind(
      std::forward<Function>(f),
      std::forward<Args>(args)...
    ));
  }
  
  thread_t(const thread_t&) = delete;
  
  ~thread_t() {
    if(joinable()) std::terminate();
  }
  
  void join() {
    if(joinable()) {
      void *dummy;
      pthread_join(_id, &dummy);
      
      _id = 0;
    }
  }

  void detach() {
    pthread_detach(_id);
    _id = 0;
  }

  bool joinable() {
    return _id != 0;
  }
  
private:
  template<class Function>
  void _startThread(Function&& callBack) {
    std::shared_ptr<gen::ThreadCallback> task = std::make_shared<_Impl<Function>>(_id, std::forward<Function>(callBack));

    threadManager->create(task);
  }
};
#ifndef DOSSIER_THREAD_T_H
#define DOSSIER_THREAD_T_H

#include <thread>

namespace util {
class _ImplBase {
public:
  //_unique_base_type _this_ptr;

  inline virtual ~_ImplBase() = default;

  virtual void run() = 0;
};

template<class Function>
class _Impl : public _ImplBase {
  Function _func;

public:

  _Impl(Function&& f) : _func(std::forward<Function>(f)) { }

  void run() {
    _func();
  }
};
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef LACKS_FEATURE_THREAD
namespace util {
typedef std::thread thread_t;
}
#else

//////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * Calling join() on std::thread causes a pure virtual function call
 * For this reason, I constructed a drop-in replacement for std::thread.
 * It contains a subset of std::thread
 */
#include <pthread.h>
#include <memory>
#include <system_error>
#include <kitty/err/err.h>

class thread_t {
  pthread_t _id;

  typedef std::unique_ptr<_ImplBase> _Runnable;
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
      )
    );
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

  bool joinable() {
    return _id != 0;
  }

private:

  static void *_main(_ImplBase *task) {
    thread_t::_Runnable _task(task);
    //auto callBack = std::move(_this->_callBack);

    _task->run();

    return nullptr;
  }

  template<class Function>
  void _startThread(Function&& callBack) {
    _ImplBase *task = new _Impl<Function>(std::forward<Function>(callBack));

    if(pthread_create(
       &_id,
       nullptr,
       reinterpret_cast<void *(*)(void *)>(&thread_t::_main),
       task
       )) {
      throw std::system_error(errno, std::system_category());
    }
  }
};
}
#endif
#endif

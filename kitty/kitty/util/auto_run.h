#ifndef KITTY_UTIL_AUTO_RUN_H
#define KITTY_UTIL_AUTO_RUN_H

#include <memory>
#include <atomic>

#include <kitty/util/utility.h>

namespace util {
template<class T> 
class AutoRun {
public:
  typedef T thread;
private:
  std::atomic<bool> _is_running;
  thread _thread;
  
public:
  AutoRun() : _is_running(false) {}
  
  template<class Start, class Middle, class End>
  void run(Start &&start, Middle &&middle, End &&end) {
    _thread = thread([this](auto &&start, auto &&middle, auto &&end) {
      start();
      
      _is_running.store(true);
      while(_is_running.load()) {
        middle();
      }
      end();
    }, std::forward<Start>(start),  std::forward<Middle>(middle),  std::forward<End>(end));
  }
  
  template<class Middle, class End>
  void run(Middle &&middle, End &&end) { run([](){}, std::forward<Middle>(middle), std::forward<End>(end)); }
  
  template<class Middle>
  void run(Middle &&middle) { run([](){}, std::forward<Middle>(middle), [](){}); }
  
  ~AutoRun() { stop(); }
  
  void stop() {
    _is_running.store(false);
  }
  
  void join() {
    if(_thread.joinable()) {
      _thread.join();
    }
  }
  
  bool isRunning() { return _is_running.load(); }
};

template<>
class AutoRun<void> {
  std::atomic<bool> _is_running;
  
  std::mutex _lock;
public:
  AutoRun() : _is_running(false) {}
  
  
  template<class Start, class Middle, class End>
  void run(Start &&start, Middle &&middle, End &&end) {
    std::lock_guard<std::mutex> lg(_lock);
    
    start();
    
    _is_running.store(true);
    while(_is_running.load()) {
      middle();
    }
    end();
  }
  
  template<class Middle, class End>
  void run(Middle &&middle, End &&end) { run([](){}, std::forward<Middle>(middle), std::forward<End>(end)); }
  
  template<class Middle>
  void run(Middle &&middle) { run([](){}, std::forward<Middle>(middle), [](){}); }
  
  ~AutoRun() { stop(); }
  
  void stop() {
    _is_running.store(false);
  }
  
  void join() {
    stop();
    
    std::lock_guard<std::mutex> lg(_lock);
  }

  bool isRunning() { return _is_running.load(); }
};

}

#endif

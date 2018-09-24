#ifndef KITTY_UTIL_AUTO_RUN_H
#define KITTY_UTIL_AUTO_RUN_H

#include <memory>
#include <atomic>
#include <chrono>

#include <kitty/util/utility.h>

namespace util {
template<class T> 
class AutoRun {
public:
  typedef T thread;
private:
  std::atomic<bool> _is_running;
  thread _loop;
  thread _hangup;
  
  std::chrono::steady_clock::time_point _loop_start;
public:
  AutoRun() : _is_running(false), _loop_start() {}
  
  template<class Start, class Middle, class End, class Hang>
  void run(Start &&start, Middle &&middle, End &&end, Hang &&hang) {
    _loop = thread([this](auto &&start, auto &&middle, auto &&end, auto &&hang) {
      start();
      
      _loop_start = std::chrono::steady_clock::now();
      _hangup = thread([this](auto &&hangup) {
        while(_is_running.load()) {
          if(_loop_start + std::chrono::seconds(1) < std::chrono::steady_clock::now()) {
            hangup();
          }
          
          std::this_thread::sleep_for(std::chrono::seconds(1));
        }
      }, std::forward<decltype(hang)>(hang));
      
      _is_running.store(true);
      while(_is_running.load()) {
        _loop_start = std::chrono::steady_clock::now();
        middle();
      }
      end();
    }, std::forward<Start>(start),  std::forward<Middle>(middle),  std::forward<End>(end), std::forward<Hang>(hang));
  }
  
  ~AutoRun() { stop(); }
  
  void stop() {
    _is_running.store(false);
  }
  
  void join() {
    if(_loop.joinable()) {
      _loop.join();
    }
    
    if(_hangup.joinable()) {
      _hangup.join();
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

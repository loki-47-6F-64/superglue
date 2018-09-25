//
// Created by loki on 14-8-18.
//

#ifndef SUPERGLUE_BLUECAST_H
#define SUPERGLUE_BLUECAST_H

#include <map>

#include <generated-src/log_interface.hpp>

#include <generated-src/blue_cast_interface.hpp>

#include <generated-src/blue_callback.hpp>
#include <generated-src/blue_view_main_callback.hpp>

#include <generated-src/blue_power_state.hpp>
#include <generated-src/blue_beacon.hpp>

#include <generated-src/permission.hpp>
#include <generated-src/log_severity.hpp>

#include <kitty/util/task_pool.h>
#include <kitty/util/auto_run.h>

#include "thread_t.hpp"

#define TASK(x,y) ::bluecast::tasks().push([x] () { y; })

namespace bluecast {

class BlueViewMainCallback;
class BlueViewDisplayCallback;

class BlueCallback : public gen::BlueCallback {
  struct beacon_t {
    gen::BlueBeacon beacon;
    util::TaskPool::task_id_t beacon_timeout_id;
  };

  util::Optional<util::TaskPool::task_id_t> _peripheral_scan_task_id;
    
  std::shared_ptr<BlueViewMainCallback> _blue_view_main_callback;
  std::shared_ptr<BlueViewDisplayCallback> _blue_view_display_callback;

  // The util::TaskPool::task_id_t is necessary for the beacon timeout
  std::map<std::string, beacon_t> _blue_beacons;
public:
  void on_blue_power_state_change(gen::BluePowerState blueState) override;
  void on_scan_result(const gen::BlueScanResult &scan) override;

  void on_gatt_services_discovered(const std::shared_ptr<gen::BlueGatt> &gatt, bool result) override;
  void on_gatt_connection_state_change(const std::shared_ptr<gen::BlueGatt> &gatt, gen::BlueGattConnectionState new_state) override;
  void on_characteristic_read(const std::shared_ptr<gen::BlueGatt> &gatt,
                              const std::shared_ptr<gen::BlueGattCharacteristic> &characteristic, bool result) override;

  void on_beacon_update(const gen::BlueBeacon &beacon) override;

  std::shared_ptr<gen::BlueViewMainCallback> on_create_main(
    const std::shared_ptr<gen::BlueViewMainController> &blue_view,
    const std::shared_ptr<gen::PermissionInterface> &permission_manager) override;

  void on_destroy_main() override;

  std::shared_ptr<gen::BlueViewDisplayCallback> on_create_display(const gen::BlueDevice& device, const std::shared_ptr<gen::BlueViewDisplayController> &blue_view) override;

  void on_destroy_display() override;

  ~BlueCallback() override;
};

void log(gen::LogSeverity severity, const std::string &message);
void request_permission(gen::Permission perm, const std::shared_ptr<gen::PermissionInterface> &permission_manager);

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

bluecast::TaskPool &tasks();
bluecast::TaskPool &tasksMainView();
bluecast::TaskPool &tasksDisplayView();

std::shared_ptr<gen::BlueController> &blueManager();
}
#endif //SUPERGLUE_BLUECAST_H

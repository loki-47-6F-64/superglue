//
// Created by loki on 14-8-18.
//

#ifndef SUPERGLUE_BLUECAST_H
#define SUPERGLUE_BLUECAST_H

#include <map>

#include <generated-src/log_interface.hpp>

#include <generated-src/blue_cast_interface.hpp>

#include <generated-src/blue_callback.hpp>

#include <generated-src/blue_power_state.hpp>
#include <generated-src/blue_beacon.hpp>

#include <generated-src/permission.hpp>
#include <generated-src/log_severity.hpp>

#include <kitty/util/task_pool.h>
#include <kitty/util/auto_run.h>

#include "task_pool.h"
#include "thread_t.hpp"

namespace bluecast {
class BlueCallback : public gen::BlueCallback {
  struct beacon_t {
    gen::BlueBeacon beacon;
    util::TaskPool::task_id_t beacon_timeout_id;
  };

  struct view_main {
    std::shared_ptr<gen::BlueViewMainController> controller;
    std::shared_ptr<gen::PermissionInterface> permission;
  };

  struct view_display {
    std::shared_ptr<gen::BlueViewDisplayController> controller;
    gen::BlueDevice device { nullptr, std::string {} };
  };

  // The util::TaskPool::task_id_t is necessary for the beacon timeout
  std::map<std::string, beacon_t> _blue_beacons;
  std::map<std::string, gen::BlueScanResult> _blue_devices;

  bool _beacon_scan_enabled { false };

  view_main    _view_main;
  view_display _view_display;

  util::TaskPool::task_id_t _peripheral_scan_task_id { nullptr };
public:
  void on_beacon_scan_enable(bool enable) override;

  void on_blue_power_state_change(gen::BluePowerState blueState) override;
  void on_scan_result(const gen::BlueScanResult &scan) override;

  void on_gatt_services_discovered(const std::shared_ptr<gen::BlueGatt> &gatt, bool result) override;
  void on_gatt_connection_state_change(const std::shared_ptr<gen::BlueGatt> &gatt, gen::BlueGattConnectionState new_state) override;
  void on_characteristic_read(const std::shared_ptr<gen::BlueGatt> &gatt,
                              const std::shared_ptr<gen::BlueGattCharacteristic> &characteristic, bool result) override;

  void on_beacon_update(const gen::BlueBeacon &beacon) override;

  void on_select_device(const gen::BlueDevice &device) override;

  void on_start_main(
    const std::shared_ptr<gen::BlueViewMainController> &blue_view,
    const std::shared_ptr<gen::PermissionInterface> &permission_manager) override;

  void on_stop_main() override;

  void on_start_display(const gen::BlueDevice& device, const std::shared_ptr<gen::BlueViewDisplayController> &blue_view) override;

  void on_stop_display() override;

  ~BlueCallback() override;


private:
  void _update_device_list();
};

void log(gen::LogSeverity severity, const std::string &message);
void request_permission(gen::Permission perm, const std::shared_ptr<gen::PermissionInterface> &permission_manager);

TaskPool &tasks();
TaskPool &tasksMainView();
TaskPool &tasksDisplayView();

std::shared_ptr<gen::BlueController> &blueManager();
}
#endif //SUPERGLUE_BLUECAST_H

//
// Created by loki on 14-8-18.
//

#ifndef SUPERGLUE_BLUECAST_H
#define SUPERGLUE_BLUECAST_H

#include <map>

#include <blue_cast_interface.hpp>

#include <blue_callback.hpp>
#include <blue_view_main_callback.hpp>

#include <blue_power_state.hpp>
#include <blue_beacon.hpp>

#include <permission.hpp>

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

util::TaskPool &tasks();
std::shared_ptr<gen::BlueController> &blueManager();

void log(gen::LogSeverity severity, const std::string &message);
void request_permission(gen::Permission perm, const std::shared_ptr<gen::PermissionInterface> &permission_manager);
}
#endif //SUPERGLUE_BLUECAST_H

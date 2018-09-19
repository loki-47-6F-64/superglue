//
// Created by loki on 14-8-18.
//

#ifndef SUPERGLUE_BLUECAST_H
#define SUPERGLUE_BLUECAST_H

#include <map>

#include <blue_cast_interface.hpp>
#include <blue_callback.hpp>
#include <blue_view_callback.hpp>
#include <blue_power_state.hpp>

namespace bluecast {

class BlueViewCallback : public gen::BlueViewCallback {
  std::shared_ptr<gen::BlueViewController> _blue_view_controller;

  bool _scan_enabled;
public:
  const std::shared_ptr<gen::BlueViewController> &get_blue_view_controller() const { return _blue_view_controller; }
  bool scan_enabled() const { return _scan_enabled; }

  BlueViewCallback() = delete;
  explicit BlueViewCallback(const std::shared_ptr<gen::BlueViewController> &view_controller) : _blue_view_controller(view_controller), _scan_enabled(false) {}

  void on_power_state_change(gen::BluePowerState new_state) override;

  void on_toggle_scan(bool scan) override;

  void on_select_device(const gen::BlueDevice &dev) override;
};


class BlueCallback : public gen::BlueCallback {
  std::shared_ptr<BlueViewCallback> _blue_view_callback;
  std::map<std::string, gen::BlueBeacon> _blue_beacons;
public:
  void on_scan_result(const gen::BlueScanResult &scan) override;

  void on_gatt_services_discovered(const std::shared_ptr<gen::BlueGatt> &gatt, bool result) override;
  void on_gatt_connection_state_change(const std::shared_ptr<gen::BlueGatt> &gatt, gen::BlueGattConnectionState new_state) override;
  void on_characteristic_read(const std::shared_ptr<gen::BlueGatt> &gatt,
                              const std::shared_ptr<gen::BlueGattCharacteristic> &characteristic, bool result) override;

  void on_beacon_update(const gen::BlueBeacon &beacon) override;

  std::shared_ptr<gen::BlueViewCallback> on_create(const std::shared_ptr<gen::BlueViewController> &blue_view,
                                                   const std::shared_ptr<gen::PermissionInterface> &permission_manager) override;

  void on_destroy() override;


  ~BlueCallback() override;
};

}
#endif //SUPERGLUE_BLUECAST_H

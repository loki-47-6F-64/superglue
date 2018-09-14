//
// Created by loki on 14-8-18.
//

#ifndef SUPERGLUE_BLUECAST_H
#define SUPERGLUE_BLUECAST_H

#include <blue_cast_interface.hpp>
#include <blue_callback.hpp>
#include <blue_view_callback.hpp>
#include <blue_power_state.hpp>

namespace bluecast {
class BlueCallback : public gen::BlueCallback {
public:

  void on_scan_result(const gen::BlueScanResult &scan) override;

  void on_gatt_services_discovered(const std::shared_ptr<gen::BlueGatt> &gatt, bool result) override;

  void on_gatt_connection_state_change(const std::shared_ptr<gen::BlueGatt> &gatt, gen::BlueGattConnectionState new_state) override;

  void on_characteristic_read(const std::shared_ptr<gen::BlueGatt> &gatt,
                              const std::shared_ptr<gen::BlueGattCharacteristic> &characteristic, bool result) override;

  ~BlueCallback() override;
};

class BlueViewCallback : public gen::BlueViewCallback {
public:
  void on_power_state_change(gen::BluePowerState new_state) override;
};

}
#endif //SUPERGLUE_BLUECAST_H

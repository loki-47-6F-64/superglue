//
// Created by loki on 20-9-18.
//

#ifndef SUPERGLUE_BLUE_VIEW_MAIN_H
#define SUPERGLUE_BLUE_VIEW_MAIN_H

#include <memory>

#include <blue_view_main_callback.hpp>

namespace gen {
class BlueViewMainController;
class PermissionInterface;
}

namespace bluecast {
class BlueViewMainCallback : public gen::BlueViewMainCallback {
  std::shared_ptr<gen::BlueViewMainController> _blue_view_main_controller;

  bool _scan_enabled;
public:
  const std::shared_ptr<gen::BlueViewMainController> &get_blue_view_controller() const { return _blue_view_main_controller; }
  bool scan_enabled() const { return _scan_enabled; }

  BlueViewMainCallback() = delete;
  explicit BlueViewMainCallback(
    const std::shared_ptr<gen::BlueViewMainController> &view_controller,
    const std::shared_ptr<gen::PermissionInterface> &permission_interface);

  void on_power_state_change(gen::BluePowerState new_state) override;

  void on_toggle_scan(bool scan) override;

  void on_select_device(const gen::BlueDevice &dev) override;
};
}

#endif //SUPERGLUE_BLUE_VIEW_MAIN_H

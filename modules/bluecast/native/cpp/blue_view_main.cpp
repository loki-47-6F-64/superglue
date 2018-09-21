//
// Created by loki on 20-9-18.
//

#include <log_severity.hpp>
#include <blue_power_state.hpp>

#include <blue_view_main_controller.hpp>
#include <blue_controller.hpp>

#include "blue_view_main.h"

#include "permission.h"
#include "bluecast.h"
namespace bluecast {

BlueViewMainCallback::BlueViewMainCallback(
  const std::shared_ptr<gen::BlueViewMainController> &view_controller,
  const std::shared_ptr<gen::PermissionInterface> &permission_manager) :_blue_view_main_controller(view_controller), _scan_enabled(false) {

  bluecast::log(gen::LogSeverity::DEBUG, permission_manager->has(gen::Permission::BLUETOOTH) ? "bluetooth::true" : "bluetooth::false");
  bluecast::log(gen::LogSeverity::DEBUG, permission_manager->has(gen::Permission::BLUETOOTH_ADMIN) ? "bluetooth_admin::true" : "bluetooth_admin::false");
  bluecast::log(gen::LogSeverity::DEBUG, permission_manager->has(gen::Permission::COARSE_LOCATION) ? "coarse_location::true" : "coarse_location::false");

  if(!permission_manager->has(gen::Permission::COARSE_LOCATION)) {
    request_permission(gen::Permission::COARSE_LOCATION, permission_manager);
  }


}
void BlueViewMainCallback::on_power_state_change(gen::BluePowerState blueState) {
  switch(blueState) {
    case gen::BluePowerState::OFF:
      log(gen::LogSeverity::DEBUG, "Bluetooth state: OFF");

      if(_scan_enabled) {
        tasks().push([](std::shared_ptr<gen::BlueViewMainController> blue_view_controller) {
          blue_view_controller->blue_enable(true);
        }, _blue_view_main_controller);
      }

      break;
    case gen::BluePowerState::TURNING_OFF:
      log(gen::LogSeverity::DEBUG, "Bluetooth state: TURNING OFF");
      break;
    case gen::BluePowerState::ON:
      log(gen::LogSeverity::DEBUG, "Bluetooth state: ON");

      if(_scan_enabled) {
        TASK(, blueManager()->scan(true));
      }
      break;
    case gen::BluePowerState::TURNING_ON:
      log(gen::LogSeverity::DEBUG, "Bluetooth state: TURNING ON");
      break;
  }
}

void BlueViewMainCallback::on_toggle_scan(bool scan) {
  _scan_enabled = scan;


  tasks().push([](auto view, auto scan) {
    if(scan && !blueManager()->is_enabled()) {
      view->blue_enable(true);
    } else {
      blueManager()->scan(scan);
    }
  }, _blue_view_main_controller, scan);
}

void BlueViewMainCallback::on_select_device(const gen::BlueDevice &dev) {
  tasks().push([this, dev]() { _blue_view_main_controller->launch_view_display(dev); });
}
/* bluecast */ }
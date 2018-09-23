//
// Created by loki on 20-9-18.
//

#include <log_severity.hpp>

#include <blue_power_state.hpp>

#include <blue_view_main_controller.hpp>
#include <blue_controller.hpp>

#include <blue_device.hpp>
#include "blue_view_main.h"

#include "permission.h"
#include "bluecast.h"
namespace bluecast {

BlueViewMainCallback::BlueViewMainCallback(
  const std::shared_ptr<gen::BlueViewMainController> &view_controller,
  const std::shared_ptr<gen::PermissionInterface> &permission_manager) :_blue_view_main_controller(view_controller) {

  bluecast::log(gen::LogSeverity::DEBUG, permission_manager->has(gen::Permission::BLUETOOTH) ? "bluetooth::true" : "bluetooth::false");
  bluecast::log(gen::LogSeverity::DEBUG, permission_manager->has(gen::Permission::BLUETOOTH_ADMIN) ? "bluetooth_admin::true" : "bluetooth_admin::false");
  bluecast::log(gen::LogSeverity::DEBUG, permission_manager->has(gen::Permission::COARSE_LOCATION) ? "coarse_location::true" : "coarse_location::false");

  if(!permission_manager->has(gen::Permission::COARSE_LOCATION)) {
    request_permission(gen::Permission::COARSE_LOCATION, permission_manager);
  }


}

void BlueViewMainCallback::on_toggle_scan(bool scan) {
  tasks().push([](auto view, auto scan) {
    if(scan && !blueManager()->is_enabled()) {
      view->blue_enable(true);
    } else {
      blueManager()->beacon_scan(scan);
    }
  }, _blue_view_main_controller, scan);
}

void BlueViewMainCallback::on_select_device(const gen::BlueDevice &dev) {
  tasks().push([this, dev]() { _blue_view_main_controller->launch_view_display(dev); });
}
/* bluecast */ }

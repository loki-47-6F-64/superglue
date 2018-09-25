//
// Created by loki on 20-9-18.
//

#include <generated-src/log_severity.hpp>

#include <generated-src/blue_power_state.hpp>

#include <generated-src/blue_view_main_controller.hpp>
#include <generated-src/blue_controller.hpp>

#include <generated-src/blue_device.hpp>
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

  _permission_manager = permission_manager;
}

void BlueViewMainCallback::on_toggle_scan(bool scan) {
  tasksMainView().push([](auto view, auto scan) {
    if(scan && !blueManager()->is_enabled()) {
      view->blue_enable(true);
    } else {
      blueManager()->beacon_scan(scan);
    }
  }, _blue_view_main_controller, scan);
}

void BlueViewMainCallback::on_search_device() {
  tasks().push([]() {
    blueManager()->peripheral_scan(true);
  });

  _peripheral_scan_task_id = tasks().pushDelayed([]() {
    blueManager()->peripheral_scan(false);
  }, std::chrono::seconds(1)).task_id;
}

const util::TaskPool::task_id_t BlueViewMainCallback::get_peripheral_scan_task_id() const {
  return _peripheral_scan_task_id;
}
/* bluecast */ }

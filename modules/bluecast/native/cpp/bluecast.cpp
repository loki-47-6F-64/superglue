//
// Created by loki on 14-8-18.
//
#include <sstream>

#include <map>

#include <generated-src/log_severity.hpp>
#include <generated-src/log_interface.hpp>

#include <generated-src/blue_cast_interface.hpp>
#include <generated-src/blue_scan_result.hpp>
#include <generated-src/blue_controller.hpp>

#include <generated-src/blue_view_main_controller.hpp>
#include <generated-src/blue_view_display_controller.hpp>

#include <generated-src/blue_beacon.hpp>

#include <generated-src/blue_gatt.hpp>
#include <generated-src/blue_gatt_service.hpp>
#include <generated-src/blue_gatt_characteristic.hpp>
#include <generated-src/blue_gatt_descriptor.hpp>
#include <generated-src/blue_gatt_connection_state.hpp>

#include <kitty/util/move_by_copy.h>
#include <kitty/util/set.h>

#include "config.hpp"
#include "bluecast.h"
#include "permission.h"

bool stringEqual(const std::string &left, const std::string &right) {
  return left.size() == right.size() && std::equal(left.cbegin(), left.cend(), right.cbegin(), [](auto l, auto r) {
    return std::toupper(l) == std::toupper(r);
  });
}

namespace bluecast {
std::shared_ptr<gen::BlueController> &blueManager() {
  static std::shared_ptr<gen::BlueController> _blueManager;

  return _blueManager;
}

TaskPool &tasks() {
  static TaskPool taskPool;

  return taskPool;
}

TaskPool &tasksMainView() {
  static TaskPool taskPool;

  return taskPool;
}

TaskPool &tasksDisplayView() {
  static TaskPool taskPool;

  return taskPool;
}

util::AutoRun<thread_t> &thread() {
  static util::AutoRun<thread_t> autoRun;

  return autoRun;
}

void log(gen::LogSeverity severity, const std::string &message) {
  logManager->log(severity, message);
}

void log_scan_result(const gen::BlueScanResult &scan) {
  std::stringstream ss;

  ss << "dev::name::" << (scan.dev.name ? *scan.dev.name : "unknown") << std::endl;
  ss << "dev::address::" << scan.dev.address << std::endl;
  ss << "dev::rssi::" << scan.rssi << std::endl;

  log(gen::LogSeverity::DEBUG, ss.str());
}

void BlueCallback::on_scan_result(const gen::BlueScanResult &scan) {
  log_scan_result(scan);

  const auto &dev = scan.dev;
    
  if(dev.name == "Viking" || dev.name == "Loki") {
    tasksMainView().push([this, scan]() {
      auto it = _blue_devices.find(scan.dev.address);

      if(it != _blue_devices.cend()) {
        auto &scan_old = it->second;

        // Attempt at accuracy with rssi while taking into account of movement
        scan_old.rssi = (scan_old.rssi + scan.rssi) / 2;
      }
      else {
        _blue_devices.emplace(scan.dev.address, scan);
      }
    });
  }
}

void BlueCallback::on_gatt_services_discovered(const std::shared_ptr<gen::BlueGatt> &gatt, bool result) {
  if(result) {
    log(gen::LogSeverity::DEBUG, "on_gatt_services_discovered::success");
    tasks().push([=]() {
      for(const auto &service : gatt->services()) {
        log(gen::LogSeverity::INFO, "service::" + service->uuid());
        for(const auto &characteristic : service->characteristics()) {
          log(gen::LogSeverity::INFO, "characteristic::" + characteristic->uuid());
          if(!stringEqual(characteristic->uuid(), "2096d612-39b6-41bf-b511-0f8ade8ef6c0")) {
            continue;
          }

          tasksDisplayView().push([gatt, characteristic]() {
            log(gen::LogSeverity::DEBUG, "gatt->read_characteristic(characteristic)");
            gatt->read_characteristic(characteristic);
          });

          for(const auto &descriptor : characteristic->descriptors()) {
            log(gen::LogSeverity::INFO, "descriptor::" + descriptor->uuid());
          }
        }
      }
    });
  }
  else {
    log(gen::LogSeverity::DEBUG, "on_gatt_services_discovered::fail");
  }
}

void BlueCallback::on_gatt_connection_state_change(const std::shared_ptr<gen::BlueGatt> &gatt, gen::BlueGattConnectionState new_state) {
  if(new_state == gen::BlueGattConnectionState::CONNECTED) {
    log(gen::LogSeverity::DEBUG, "on_gatt_connection_result::CONNECTED");
    bluecast::tasksDisplayView().push([gatt]() { gatt->discover_services(); });
  }
  else if(new_state == gen::BlueGattConnectionState::DISCONNECTED) {
    log(gen::LogSeverity::DEBUG, "on_gatt_connection_result::DISCONNECTED");
  }
}

void BlueCallback::on_characteristic_read(const std::shared_ptr<gen::BlueGatt> &gatt,
                                          const std::shared_ptr<gen::BlueGattCharacteristic> &characteristic,
                                          bool result) {
  log(gen::LogSeverity::DEBUG, "on_characteristic_read::" + std::string(result ? "success" : "fail"));

  tasks().push([gatt]() { gatt->disconnect(); });
  if(result) {
    log(gen::LogSeverity::INFO, "value::" + characteristic->get_string_value(0));
  }

  tasksDisplayView().push([this, characteristic]() {
    _view_display.controller->display(_view_display.device, characteristic->get_string_value(0));
  });
}

void BlueCallback::on_create_main(
  const std::shared_ptr<gen::BlueViewMainController> &blue_view,
  const std::shared_ptr<gen::PermissionInterface> &permission_manager) {

  log(gen::LogSeverity::DEBUG, "on_create_main");

  bluecast::log(gen::LogSeverity::DEBUG,
                permission_manager->has(gen::Permission::BLUETOOTH) ? "bluetooth::true" : "bluetooth::false");

  bluecast::log(gen::LogSeverity::DEBUG,
                permission_manager->has(gen::Permission::BLUETOOTH_ADMIN) ? "bluetooth_admin::true"
                                                                          : "bluetooth_admin::false");
  bluecast::log(gen::LogSeverity::DEBUG,
                permission_manager->has(gen::Permission::COARSE_LOCATION) ? "coarse_location::true"
                                                                          : "coarse_location::false");

  if(!permission_manager->has(gen::Permission::COARSE_LOCATION)) {
    request_permission(gen::Permission::COARSE_LOCATION, permission_manager);
  }

  _view_main = { blue_view, permission_manager };

  tasksMainView().start();

  tasks().push([this]() {
    if(blueManager()->is_enabled()) {
      _update_device_list();
    }
  });
}

void BlueCallback::on_create_display(
  const gen::BlueDevice& device,
  const std::shared_ptr<gen::BlueViewDisplayController> &blue_view) {

  log(gen::LogSeverity::DEBUG, "on_create_display");

  _view_display = { blue_view, device };

  tasksDisplayView().start();

  tasksDisplayView().push([device]() { blueManager()->connect_gatt(device); });
}

void BlueCallback::on_destroy_main() {
  log(gen::LogSeverity::DEBUG, "on_destroy_main");

  tasksMainView().clear();
  _view_main.controller.reset();
}

void BlueCallback::on_destroy_display() {
  log(gen::LogSeverity::DEBUG, "on_destroy_display");

  tasksDisplayView().clear();
  _view_display.controller.reset();
}

void BlueCallback::on_beacon_update(const gen::BlueBeacon &beacon) {
  tasks().push([this](gen::BlueBeacon &&beacon) {
    auto it = _blue_beacons.find(beacon.uuid);

    util::TaskPool::task_id_t task_id;

    // If beacon first appears...
    if(it == _blue_beacons.cend()) {
      log(gen::LogSeverity::DEBUG, "detected new beacon in range");

      _update_device_list();
      // Detect beacons that are out of range
      auto delayed_task = tasks().pushDelayed([this, beacon]() {
        log(gen::LogSeverity::DEBUG, "detected beacon out of range");
        _update_device_list();

        _blue_beacons.erase(beacon.uuid);
      }, std::chrono::seconds(2));

      task_id = delayed_task.task_id;
    }
    else {
      task_id = it->second.beacon_timeout_id;
      tasks().delay(task_id, std::chrono::seconds(2));
    }

    _blue_beacons.emplace(beacon.uuid, beacon_t { beacon, task_id });

  }, util::const_cmove(beacon));
}

BlueCallback::~BlueCallback() = default;

void request_permission(gen::Permission perm, const std::shared_ptr<gen::PermissionInterface> &permission_manager) {
  permission_manager->request(
    perm,
    std::make_shared<PermFunc>([perm, permission_manager](gen::Permission p, bool granted) {
      if(!granted) {
        // Keep pestering the user for permission
        tasks().push(request_permission, perm, permission_manager);
      }
    }));
}

void BlueCallback::on_blue_power_state_change(gen::BluePowerState blueState) {
  switch(blueState) {
    case gen::BluePowerState::OFF:
      log(gen::LogSeverity::DEBUG, "Bluetooth state: OFF");
      tasksMainView().push([this]() {
        if(_beacon_scan_enabled) {
          _view_main.controller->blue_enable(true);
        }
      });

      break;
    case gen::BluePowerState::ON:
      // TODO: make sure searching for beacons resumes automatically
      log(gen::LogSeverity::DEBUG, "Bluetooth state: ON");
      break;
  }
}

void BlueCallback::on_beacon_scan_enable(bool enable) {
  tasksMainView().push([this, enable]() {
    _beacon_scan_enabled = enable;
    if(enable && !blueManager()->is_enabled()) {
      _view_main.controller->blue_enable(true);
    } else {
      blueManager()->beacon_scan(enable);
    }
  });
}

void BlueCallback::on_select_device(const gen::BlueDevice &device) {
  tasksMainView().push([this, device]() { _view_main.controller->launch_view_display(device); });
}

void BlueCallback::_update_device_list() {
  tasksMainView().push([this]() {
    if(_peripheral_scan_task_id) {
      tasks().delay(_peripheral_scan_task_id, std::chrono::seconds(1));
    }

    blueManager()->peripheral_scan(true);
    tasks().pushDelayed([this]() {
      _peripheral_scan_task_id = nullptr;
      blueManager()->peripheral_scan(false);

      tasksMainView().push([this]() {
        auto scan_list = util::map(_blue_devices, [](auto &el) { return el.second; });

        std::sort(std::begin(scan_list), std::end(scan_list), [](auto &l, auto &r) { return l.rssi < r.rssi; });


        auto device_list = util::map(std::begin(scan_list), std::begin(scan_list) + _blue_beacons.size(), [](auto &el) { return el.dev; });
        _view_main.controller->set_device_list(device_list);
      });
    }, std::chrono::seconds(1));
  });
}

/* namespace bluecast */ }

std::shared_ptr<gen::BlueCallback> gen::BlueCastInterface::config(
  const std::shared_ptr<gen::BlueController> &blue_manager) {
  bluecast::blueManager() = blue_manager;

  bluecast::thread().run(
    [] () {
      bluecast::tasks().start();
      bluecast::log(LogSeverity::DEBUG, "started main superglue loop");
    },
    []() {
      auto &tasks            = bluecast::tasks();
      auto &tasksMainView    = bluecast::tasksMainView();
      auto &tasksDisplayView = bluecast::tasksDisplayView();


      while(tasks.ready() || tasksMainView.ready() || tasksDisplayView.ready()) {
        if(auto task = tasks.pop()) {
          tasks.run(task);
        }

        if(auto task = tasksMainView.pop()) {
          tasksMainView.run(task);
        }

        if(auto task = tasksDisplayView.pop()) {
          tasksDisplayView.run(task);
        }
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    },
    []() {
      bluecast::tasks().clear();
    },
    []() {
      bluecast::log(gen::LogSeverity::WARN, "bluecast::thread() not responding");
    });

  return std::make_shared<bluecast::BlueCallback>();
}

//
// Created by loki on 14-8-18.
//
#include <sstream>

#include <map>

#include <blue_cast_interface.hpp>
#include <log_severity.hpp>
#include <blue_scan_result.hpp>
#include <blue_controller.hpp>

#include <blue_view_main_controller.hpp>
#include <blue_view_display_controller.hpp>

#include <blue_beacon.hpp>

#include <blue_gatt.hpp>
#include <blue_gatt_service.hpp>
#include <blue_gatt_characteristic.hpp>
#include <blue_gatt_descriptor.hpp>
#include <blue_gatt_connection_state.hpp>

#include <kitty/util/move_by_copy.h>

#include "config.hpp"
#include "bluecast.h"
#include "permission.h"

#include "blue_view_main.h"
#include "blue_view_display.h"

namespace bluecast {
std::shared_ptr<gen::BlueController> &blueManager() {
  static std::shared_ptr<gen::BlueController> _blueManager;

  return _blueManager;
}

util::TaskPool &tasks() {
  static util::TaskPool taskPool;

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

  ss << std::endl;
  ss << "data_complete::" << scan.data_complete << std::endl;
  ss << "connectable::" << scan.connectable << std::endl;
  if (scan.advertising_sid) {
    ss << "advertising_sid::" << *scan.advertising_sid << std::endl;
  }

  if (scan.tx_power) {
    ss << "tx_power::" << *scan.tx_power << std::endl;
  }

  ss << "rssi::" << scan.rssi << std::endl;
  ss << "dev::name::" << (scan.dev.name ? *scan.dev.name : "unknown") << std::endl;
  ss << "dev::address::" << scan.dev.address << std::endl;

  log(gen::LogSeverity::DEBUG, ss.str());
}

void BlueCallback::on_scan_result(const gen::BlueScanResult &scan) {
  log_scan_result(scan);

  tasks().push([this](gen::BlueScanResult &&scan) {
    auto &dev = scan.dev;

    auto it = _blue_beacons.find(dev.address);
    if(it != _blue_beacons.end() && it->second.beacon.device.name != dev.name) {
      it->second.beacon.device = std::move(dev);

      _blue_view_main_callback->get_blue_view_controller()->beacon_list_update(it->second.beacon);
    }
  }, util::cmove(const_cast<gen::BlueScanResult&>(scan)));
}

void BlueCallback::on_gatt_services_discovered(const std::shared_ptr<gen::BlueGatt> &gatt, bool result) {
  if(result) {
    log(gen::LogSeverity::DEBUG, "on_gatt_services_discovered::success");
    TASK(=,
      for(const auto &service : gatt->services()) {
        log(gen::LogSeverity::INFO, "service::" + service->uuid());
        for(const auto &characteristic : service->characteristics()) {
          log(gen::LogSeverity::INFO, "characteristic::" + characteristic->uuid());
          if(characteristic->uuid() != "2096d612-39b6-41bf-b511-0f8ade8ef6c0") {
            continue;
          }

          TASK(=, gatt->read_characteristic(characteristic));

          for(const auto &descriptor : characteristic->descriptors()) {
            log(gen::LogSeverity::INFO, "descriptor::" + descriptor->uuid());
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
    TASK(gatt, gatt->discover_services());
  }
  else if(new_state == gen::BlueGattConnectionState::DISCONNECTED) {
    log(gen::LogSeverity::DEBUG, "on_gatt_connection_result::DISCONNECTED");
    TASK(gatt, gatt->close());
  }
  else {
    log(gen::LogSeverity::DEBUG, "on_gatt_connection_result::NOT_CONNECTED");
  }
}

void BlueCallback::on_characteristic_read(const std::shared_ptr<gen::BlueGatt> &gatt,
                                          const std::shared_ptr<gen::BlueGattCharacteristic> &characteristic,
                                          bool result) {
  log(gen::LogSeverity::DEBUG, "on_characteristic_read::" + std::string(result ? "success" : "fail"));

  TASK(=, gatt->disconnect());
  if(result) {
    log(gen::LogSeverity::INFO, "value::" + characteristic->get_string_value(0));
  }


  _blue_view_display_callback->blue_view_display_controller()->display(_blue_view_display_callback->get_device(),characteristic->get_string_value(0));
}

std::shared_ptr<gen::BlueViewMainCallback> BlueCallback::on_create_main(
  const std::shared_ptr<gen::BlueViewMainController> &blue_view,
  const std::shared_ptr<gen::PermissionInterface> &permission_manager) {

  log(gen::LogSeverity::DEBUG, "on_create_main");

  _blue_view_main_callback = std::make_shared<BlueViewMainCallback>(blue_view, permission_manager);

  tasks().push([this]() {
    auto &control = _blue_view_main_callback->get_blue_view_controller();
    for(const auto &beacon : _blue_beacons) {
      control->beacon_list_update(beacon.second.beacon);
    }
  });

  return _blue_view_main_callback;
}

std::shared_ptr<gen::BlueViewDisplayCallback> BlueCallback::on_create_display(
  const gen::BlueDevice& device,
  const std::shared_ptr<gen::BlueViewDisplayController> &blue_view) {

  log(gen::LogSeverity::DEBUG, "on_create_display");

  _blue_view_display_callback = std::make_shared<BlueViewDisplayCallback>(blue_view, device);

  blueManager()->connect_gatt(device);

  return _blue_view_display_callback;
}

void BlueCallback::on_destroy_main() {
  log(gen::LogSeverity::DEBUG, "on_destroy_main");
  if(_blue_view_main_callback->scan_enabled()) {
    TASK(,blueManager()->scan(false));
  }

  _blue_view_main_callback.reset();
}

void BlueCallback::on_destroy_display() {
  log(gen::LogSeverity::DEBUG, "on_destroy_display");
  _blue_view_display_callback.reset();
}

void BlueCallback::on_beacon_update(const gen::BlueBeacon &beacon) {
  auto &_beacon = const_cast<gen::BlueBeacon&>(beacon);

  tasks().push([this](gen::BlueBeacon &&beacon) {
    _blue_view_main_callback->get_blue_view_controller()->beacon_list_update(beacon);

    auto it = _blue_beacons.find(beacon.device.address);

    util::TaskPool::task_id_t task_id;
    // If beacon first appears...
    if(it == _blue_beacons.cend()) {
      auto delayed_task = tasks().pushTimed([this](const auto &beacon) {
        _blue_view_main_callback->get_blue_view_controller()->beacon_list_remove(beacon);

        _blue_beacons.erase(beacon.address);
      }, std::chrono::seconds(3), beacon.device);

      task_id = delayed_task.task_id;
    }
    else {
      task_id = it->second.beacon_timeout_id;
      tasks().delayTask(task_id, std::chrono::seconds(3));
    }

    std::string address = beacon.device.address;
    _blue_beacons.emplace(address, beacon_t { std::move(beacon), task_id });

  }, util::cmove(_beacon));
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

/* namespace bluecast */ }

std::shared_ptr<gen::BlueCallback> gen::BlueCastInterface::config(
  const std::shared_ptr<gen::BlueController> &blue_manager) {
  bluecast::blueManager() = blue_manager;

  bluecast::thread().run(
    [] () {
      bluecast::log(LogSeverity::DEBUG, "started main superglue loop");
    },
    []() {
      while (auto task = bluecast::tasks().pop()) {
        auto _begin = std::chrono::steady_clock::now();
        (*task)->run();
        auto milli = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - _begin);

        if(milli.count() > 100) {
          logManager->log(gen::LogSeverity::WARN, "duration of task => [" + std::to_string(milli.count()) + "] milliseconds");
        }
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }, []() {});

  return std::make_shared<bluecast::BlueCallback>();
}
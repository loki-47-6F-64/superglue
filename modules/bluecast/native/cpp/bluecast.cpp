//
// Created by loki on 14-8-18.
//
#include <sstream>

#include <blue_cast_interface.hpp>
#include <log_severity.hpp>
#include <blue_scan_result.hpp>
#include <blue_controller.hpp>

#include <blue_gatt.hpp>
#include <blue_gatt_service.hpp>
#include <blue_gatt_characteristic.hpp>
#include <blue_gatt_descriptor.hpp>

#include <permission.hpp>

#include <kitty/util/auto_run.h>
#include <kitty/util/thread_pool.h>
#include <generated-src/blue_gatt_connection_state.hpp>

#include "config.hpp"
#include "thread_t.hpp"
#include "bluecast.h"
#include "permission.h"

#define TASK(x,y) tasks().push([x] () { y; })
namespace bluecast {
std::shared_ptr<gen::BlueController> blueManager;

util::TaskPool &tasks() {
  static util::TaskPool taskPool;

  return taskPool;
}

util::AutoRun<thread_t> &thread() {
  static util::AutoRun<thread_t> autoRun;

  return autoRun;
}

void BlueCallback::on_state_change(gen::BluePowerState blueState) {
  switch (blueState) {
    case gen::BluePowerState::OFF:
      logManager->log(gen::LogSeverity::DEBUG, "Bluetooth state: OFF");

      blueManager->enable();
      break;
    case gen::BluePowerState::TURNING_OFF:
      logManager->log(gen::LogSeverity::DEBUG, "Bluetooth state: TURNING OFF");
      break;
    case gen::BluePowerState::ON:
      logManager->log(gen::LogSeverity::DEBUG, "Bluetooth state: ON");

      blueManager->scan(true);
      break;
    case gen::BluePowerState::TURNING_ON:
      logManager->log(gen::LogSeverity::DEBUG, "Bluetooth state: TURNING ON");
      break;
  }
}

void BlueCallback::on_scan_result(const gen::BlueScanResult &scan) {
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

  if(scan.dev.name && *scan.dev.name == "Viking") {
    TASK(scan,
      bluecast::blueManager->scan(false);
      bluecast::blueManager->connect_gatt(scan.dev);
    );
  }

  logManager->log(gen::LogSeverity::DEBUG, ss.str());
}

void BlueCallback::on_gatt_services_discovered(const std::shared_ptr<gen::BlueGatt> &gatt, bool result) {
  if(result) {
    logManager->log(gen::LogSeverity::DEBUG, "on_gatt_services_discovered::success");
    TASK(=,
      for(const auto &service : gatt->services()) {
        logManager->log(gen::LogSeverity::INFO, "service::" + service->uuid());
        for(const auto &characteristic : service->characteristics()) {
          logManager->log(gen::LogSeverity::INFO, "characteristic::" + characteristic->uuid());
          if(characteristic->uuid() != "2096d612-39b6-41bf-b511-0f8ade8ef6c0") {
            continue;
          }

          TASK(=, gatt->read_characteristic(characteristic));

          for(const auto &descriptor : characteristic->descriptors()) {
            logManager->log(gen::LogSeverity::INFO, "descriptor::" + descriptor->uuid());
          }
        }
      }
    );
  }
  else {
    logManager->log(gen::LogSeverity::DEBUG, "on_gatt_services_discovered::fail");
  }
}

void BlueCallback::on_gatt_connection_state_change(const std::shared_ptr<gen::BlueGatt> &gatt, gen::BlueGattConnectionState new_state) {
  if(new_state == gen::BlueGattConnectionState::CONNECTED) {
    logManager->log(gen::LogSeverity::DEBUG, "on_gatt_connection_result::CONNECTED");
    TASK(=, gatt->discover_services());
  }
  else if(new_state == gen::BlueGattConnectionState::DISCONNECTED) {
    logManager->log(gen::LogSeverity::DEBUG, "on_gatt_connection_result::DISCONNECTED");
  }
  else {
    logManager->log(gen::LogSeverity::DEBUG, "on_gatt_connection_result::NOT_CONNECTED");
  }
}

void BlueCallback::on_characteristic_read(const std::shared_ptr<gen::BlueGatt> &gatt,
                                          const std::shared_ptr<gen::BlueGattCharacteristic> &characteristic,
                                          bool result) {
  logManager->log(gen::LogSeverity::DEBUG, "on_characteristic_read::" + std::string(result ? "success" : "fail"));

  TASK(=, gatt->disconnect());
  if(result) {
    TASK(=,
      logManager->log(gen::LogSeverity::INFO, "value::" + characteristic->get_string_value(0))
    );
  }
}


BlueCallback::~BlueCallback() = default;

/* namespace bluecast */ }

void gen::BlueCastInterface::config(
  const std::shared_ptr<gen::BlueController> &blue_manager,
  const std::shared_ptr<gen::PermissionInterface> &permission_manager) {
  bluecast::blueManager = blue_manager;

  if (!bluecast::blueManager->is_enabled()) {
    bluecast::blueManager->enable();
  }

  logManager->log(LogSeverity::DEBUG, permission_manager->has(gen::Permission::BLUETOOTH) ? "bluetooth::true" : "bluetooth::false");
  logManager->log(LogSeverity::DEBUG, permission_manager->has(gen::Permission::BLUETOOTH_ADMIN) ? "bluetooth_admin::true" : "bluetooth_admin::false");
  logManager->log(LogSeverity::DEBUG, permission_manager->has(gen::Permission::COARSE_LOCATION) ? "coarse_location::true" : "coarse_location::false");

  bluecast::thread().run(
    [] () {
      logManager->log(LogSeverity::DEBUG, "started main superglue loop");
    },
    []() {
      while (auto task = bluecast::tasks().pop()) {
        (*task)->run();
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }, []() {});

  bluecast::tasks().push([permission_manager]() {
    permission_manager->request(gen::Permission::COARSE_LOCATION,
      std::make_shared<PermFunc>([permission_manager](Permission p, bool granted) {
        if (permission_manager->has(gen::Permission::COARSE_LOCATION)) {
          logManager->log(LogSeverity::DEBUG, "coarse_location::true");

          logManager->log(LogSeverity::INFO, "start scanning.");
          bluecast::blueManager->scan(true);
          bluecast::tasks().pushTimed([]() {
            logManager->log(LogSeverity::INFO, "stop scanning.");
            bluecast::blueManager->scan(false);
            }, 100000);
        } else {
          logManager->log(LogSeverity::DEBUG, "coarse_location::false");
        }
      }));
  });
}

std::shared_ptr<gen::BlueCallback> gen::BlueCastInterface::get_callback() {
  return std::make_shared<bluecast::BlueCallback>();
}
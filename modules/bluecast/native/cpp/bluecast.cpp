//
// Created by loki on 14-8-18.
//
#include <sstream>

#include <map>

#include <blue_cast_interface.hpp>
#include <log_severity.hpp>
#include <blue_scan_result.hpp>
#include <blue_controller.hpp>
#include <blue_view_controller.hpp>

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

#define TASK(x,y) ::bluecast::tasks().push([x] () { y; })
#define DELAY(x,y,z) ::bluecast::tasks().pushTimed([x] () { y; }, std::chrono::milliseconds(z))
namespace bluecast {
std::shared_ptr<gen::BlueController> blueManager;
std::shared_ptr<gen::BlueViewController> blueView;

auto &blue_devices() {
  static std::map<std::string, gen::BlueDevice> _blue_devices;

  return _blue_devices;
}

auto &tasks() {
  static util::TaskPool taskPool;

  return taskPool;
}

auto &thread() {
  static util::AutoRun<thread_t> autoRun;

  return autoRun;
}

void log(gen::LogSeverity severity, const std::string &message) {
  logManager->log(severity, message);
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

  TASK(scan,
    if(blue_devices().count(scan.dev.address) == 0) {
      if(scan.dev.name && *scan.dev.name == "Viking") {
        bluecast::blueManager->scan(false);

        bluecast::blueManager->connect_gatt(scan.dev);
      }

      blue_devices().emplace(scan.dev.address, scan.dev);
    }
  );


  log(gen::LogSeverity::DEBUG, ss.str());
}

void BlueCallback::on_gatt_services_discovered(const std::shared_ptr<gen::BlueGatt> &gatt, bool result) {
  if(result) {
    log(gen::LogSeverity::DEBUG, "on_gatt_services_discovered::success");
    TASK(=,
      for(const auto &service : gatt->services()) {
        log(gen::LogSeverity::INFO, "service::" + service->uuid());
        for(const auto &characteristic : service->characteristics()) {
          log(gen::LogSeverity::INFO, "characteristic::" + characteristic->uuid());
          if(characteristic->uuid() != "00002a00-0000-1000-8000-00805f9b34fb") {
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
}


BlueCallback::~BlueCallback() = default;

void BlueViewCallback::on_power_state_change(gen::BluePowerState blueState) {
  switch (blueState) {
    case gen::BluePowerState::OFF:
      log(gen::LogSeverity::DEBUG, "Bluetooth state: OFF");

      TASK(,blueView->blue_enable(true));
      break;
    case gen::BluePowerState::TURNING_OFF:
      log(gen::LogSeverity::DEBUG, "Bluetooth state: TURNING OFF");
      break;
    case gen::BluePowerState::ON:
      log(gen::LogSeverity::DEBUG, "Bluetooth state: ON");

      TASK(,blueManager->scan(true));
      break;
    case gen::BluePowerState::TURNING_ON:
      log(gen::LogSeverity::DEBUG, "Bluetooth state: TURNING ON");
      break;
  }
}
/* namespace bluecast */ }

void start_scan() {
  bluecast::log(gen::LogSeverity::INFO, "start scanning.");
  bluecast::blueManager->scan(true);
  bluecast::tasks().pushTimed([]() {
    bluecast::blueManager->scan(false);
  }, std::chrono::seconds(10));
}

std::shared_ptr<gen::BlueViewCallback> gen::BlueCastInterface::on_create(
  const std::shared_ptr<gen::BlueViewController> &blue_view,
  const std::shared_ptr<gen::PermissionInterface> &permission_manager) {

  bluecast::log(LogSeverity::DEBUG, permission_manager->has(gen::Permission::BLUETOOTH) ? "bluetooth::true" : "bluetooth::false");
  bluecast::log(LogSeverity::DEBUG, permission_manager->has(gen::Permission::BLUETOOTH_ADMIN) ? "bluetooth_admin::true" : "bluetooth_admin::false");
  bluecast::log(LogSeverity::DEBUG, permission_manager->has(gen::Permission::COARSE_LOCATION) ? "coarse_location::true" : "coarse_location::false");

  bluecast::blueView = blue_view;
  if (!bluecast::blueManager->is_enabled()) {
    bluecast::blueView->blue_enable(true);

    return std::make_shared<bluecast::BlueViewCallback>();
  }

  if(!permission_manager->has(gen::Permission::COARSE_LOCATION)) {
    bluecast::tasks().push([permission_manager]() {
      permission_manager->request(
        gen::Permission::COARSE_LOCATION,
        std::make_shared<PermFunc>([](Permission p, bool granted) {
          if(granted) {
            bluecast::log(LogSeverity::DEBUG, "coarse_location::true");

            start_scan();
          } else {
            bluecast::log(LogSeverity::DEBUG, "coarse_location::false");
          }
        }));
    });
  } else {
    TASK(,start_scan());
  }

  return std::make_shared<bluecast::BlueViewCallback>();
}

void gen::BlueCastInterface::on_destroy() { }

void gen::BlueCastInterface::config(
  const std::shared_ptr<gen::BlueController> &blue_manager) {
  bluecast::blueManager = blue_manager;



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
}

std::shared_ptr<gen::BlueCallback> gen::BlueCastInterface::get_callback() {
  return std::make_shared<bluecast::BlueCallback>();
}

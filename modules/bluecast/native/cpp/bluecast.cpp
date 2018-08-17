//
// Created by loki on 14-8-18.
//
#include <sstream>

#include <blue_cast_interface.hpp>
#include <log_severity.hpp>
#include <blue_scan_result.hpp>
#include <blue_controller.hpp>

#include <permission.hpp>

#include <kitty/util/auto_run.h>
#include <kitty/util/thread_pool.h>

#include "config.hpp"
#include "thread_t.hpp"
#include "bluecast.h"
#include "permission.h"

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
  ss << "dev::name::" << scan.dev.name << std::endl;
  ss << "dev::address::" << scan.dev.address << std::endl;

  logManager->log(gen::LogSeverity::DEBUG, ss.str());
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

  bluecast::tasks().push([=]() {
    permission_manager->request(gen::Permission::COARSE_LOCATION,
      std::make_shared<PermFunc>([=, &permission_manager](Permission p, bool granted) {
        if (permission_manager->has(gen::Permission::COARSE_LOCATION)) {
          logManager->log(LogSeverity::DEBUG, "coarse_location::true");
        } else {
          logManager->log(LogSeverity::DEBUG, "coarse_location::false");
        }
      }));
  });
}

std::shared_ptr<gen::BlueCallback> gen::BlueCastInterface::get_callback() {
  return std::make_shared<bluecast::BlueCallback>();
}
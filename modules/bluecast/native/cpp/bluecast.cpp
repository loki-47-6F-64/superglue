//
// Created by loki on 14-8-18.
//

#include <blue_cast_interface.hpp>
#include <log_severity.hpp>
#include "config.hpp"
#include "bluecast.h"

void gen::BlueCastInterface::config() {

}

std::shared_ptr<gen::BlueCallback> gen::BlueCastInterface::get_callback() {
  std::shared_ptr<BlueCallback> tmp = std::make_shared<::BlueCallback>();

  return tmp;
}

void BlueCallback::on_state_change(gen::BlueState blueState) {
  switch(blueState) {
    case gen::BlueState::OFF:
      logManager->log(gen::LogSeverity::DEBUG, "Bluetooth state: OFF");
      break;
    case gen::BlueState::TURNING_OFF:
      logManager->log(gen::LogSeverity::DEBUG, "Bluetooth state: TURNING OFF");
      break;
    case gen::BlueState::ON:
      logManager->log(gen::LogSeverity::DEBUG, "Bluetooth state: ON");
      break;
    case gen::BlueState::TURNING_ON:
      logManager->log(gen::LogSeverity::DEBUG, "Bluetooth state: TURNING ON");
      break;
  }
}

BlueCallback::~BlueCallback() = default;

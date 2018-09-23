//
// Created by loki on 20-9-18.
//

#include "blue_view_display.h"

const std::shared_ptr<gen::BlueViewDisplayController> &bluecast::BlueViewDisplayCallback::blue_view_display_controller() const {
  return _blue_view_display_controller;
}

bluecast::BlueViewDisplayCallback::BlueViewDisplayCallback(
  const std::shared_ptr<gen::BlueViewDisplayController> &blue_view_display_controller,
  const gen::BlueDevice &device)
  : _blue_view_display_controller(blue_view_display_controller), _device(device) { }

const gen::BlueDevice &bluecast::BlueViewDisplayCallback::get_device() const {
  return _device;
}

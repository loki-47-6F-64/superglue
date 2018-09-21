//
// Created by loki on 20-9-18.
//

#ifndef SUPERGLUE_BLUE_VIEW_DISPLAY_H
#define SUPERGLUE_BLUE_VIEW_DISPLAY_H

#include <memory>
#include <blue_view_display_callback.hpp>

#include <blue_device.hpp>

namespace gen {
class BlueViewDisplayController;
}
namespace bluecast {
class BlueViewDisplayCallback : public gen::BlueViewDisplayCallback {
private:
  std::shared_ptr<gen::BlueViewDisplayController> _blue_view_display_controller;

  const gen::BlueDevice _device;
public:
  const std::shared_ptr<gen::BlueViewDisplayController> &blue_view_display_controller() const;

  const gen::BlueDevice &get_device() const;

  explicit BlueViewDisplayCallback(const std::shared_ptr<gen::BlueViewDisplayController> &blue_view_display_controller, const gen::BlueDevice &device);
};
}
#endif //SUPERGLUE_BLUE_VIEW_DISPLAY_H

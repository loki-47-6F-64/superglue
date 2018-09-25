//
// Created by loki on 20-9-18.
//

#ifndef SUPERGLUE_BLUE_VIEW_MAIN_H
#define SUPERGLUE_BLUE_VIEW_MAIN_H

#include <memory>

#include <generated-src/blue_view_main_callback.hpp>

#include <kitty/util/task_pool.h>

namespace gen {
class BlueViewMainController;
class PermissionInterface;
}

namespace bluecast {
class BlueViewMainCallback : public gen::BlueViewMainCallback {
  std::shared_ptr<gen::BlueViewMainController> _blue_view_main_controller;
  std::shared_ptr<gen::PermissionInterface> _permission_manager;

  util::TaskPool::task_id_t _peripheral_scan_task_id;
public:
  const std::shared_ptr<gen::BlueViewMainController> &get_blue_view_controller() const { return _blue_view_main_controller; }
  const util::TaskPool::task_id_t get_peripheral_scan_task_id() const;

  BlueViewMainCallback() = delete;
  explicit BlueViewMainCallback(
    const std::shared_ptr<gen::BlueViewMainController> &view_controller,
    const std::shared_ptr<gen::PermissionInterface> &permission_interface);
    
  void on_toggle_scan(bool scan) override;

  void on_search_device() override;
};
}

#endif //SUPERGLUE_BLUE_VIEW_MAIN_H

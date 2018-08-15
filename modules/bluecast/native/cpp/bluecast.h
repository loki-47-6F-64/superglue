//
// Created by loki on 14-8-18.
//

#ifndef SUPERGLUE_BLUECAST_H
#define SUPERGLUE_BLUECAST_H

#include <blue_cast_interface.hpp>
#include <blue_callback.hpp>
#include <blue_state.hpp>

class BlueCallback : public gen::BlueCallback {
public:
  void on_state_change(gen::BlueState blueState) override;

  ~BlueCallback() override;
};

#endif //SUPERGLUE_BLUECAST_H

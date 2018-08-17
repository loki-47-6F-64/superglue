//
// Created by loki on 17-8-18.
//

#ifndef SUPERGLUE_PERMISSION_H
#define SUPERGLUE_PERMISSION_H

#include <type_traits>
#include <functional>

#include <permission_callback.hpp>
#include <permission_interface.hpp>

class PermFunc : public gen::PermissionCallback {

public:
  template<typename T>
  PermFunc(T &&f) : _func(f) {};

  void result(gen::Permission perm, bool granted) override;
private:
  std::function<void(gen::Permission, bool)> _func;
};

#endif //SUPERGLUE_PERMISSION_H

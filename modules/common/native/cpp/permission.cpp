//
// Created by loki on 17-8-18.
//

#include "permission.h"

void PermFunc::result(gen::Permission perm, bool granted) {
  _func(perm, granted);
}
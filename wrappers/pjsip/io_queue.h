//
// Created by loki on 25-1-19.
//

#ifndef T_MAN_IOQUEUE_H
#define T_MAN_IOQUEUE_H

#include "nath.h"
#include "ice_trans.h"

namespace pj {
class IOQueue {
public:
  IOQueue() = default;
  IOQueue(pool_t &pool, ice_trans_cfg_t &ice_trans_cfg);

  int poll(std::chrono::milliseconds &duration);

  io_queue_t::pointer raw() const;
private:
  io_queue_t _io_queue;
};
}

#endif //T_MAN_IOQUEUE_H

//
// Created by loki on 25-1-19.
//

#include "io_queue.h"

namespace pj {

constexpr auto IOQUEUE_MAX_HANDLES = 16 > PJ_IOQUEUE_MAX_HANDLES ? PJ_IOQUEUE_MAX_HANDLES : 16;

IOQueue::IOQueue(pool_t &pool, ice_trans_cfg_t &ice_trans_cfg) {
  pj_ioqueue_create(pool.get(), IOQUEUE_MAX_HANDLES, &ice_trans_cfg.stun_cfg.ioqueue);

  _io_queue.reset(ice_trans_cfg.stun_cfg.ioqueue);
}

io_queue_t::pointer IOQueue::raw() const {
  return _io_queue.get();
}

int IOQueue::poll(std::chrono::milliseconds &duration) {
  auto timeout = time(duration);

  auto c = pj_ioqueue_poll(_io_queue.get(), &timeout);

  duration = time(timeout);

  return c;
}

}

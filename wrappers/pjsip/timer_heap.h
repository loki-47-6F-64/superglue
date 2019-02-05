//
// Created by loki on 25-1-19.
//

#ifndef T_MAN_TIMERHEAP_H
#define T_MAN_TIMERHEAP_H

#include <memory>
#include "nath.h"
#include "ice_trans.h"

namespace pj {

class TimerHeap {
public:
  TimerHeap() = default;
  TimerHeap(pool_t &pool, ice_trans_cfg_t &ice_trans_cfg);

  unsigned poll(std::chrono::milliseconds &duration);

  timer_heap_t::pointer raw() const;
private:
  timer_heap_t _timer_heap;
};
}

#endif //T_MAN_TIMERHEAP_H

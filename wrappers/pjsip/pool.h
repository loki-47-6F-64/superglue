//
// Created by loki on 24-1-19.
//

#ifndef T_MAN_POOL_H
#define T_MAN_POOL_H

#include "nath.h"
#include "timer_heap.h"
#include "io_queue.h"
#include "ice_trans.h"
#include "dns_resolv.h"

namespace pj {

// TODO :: Configure [TURN]
class Pool {
public:
  Pool() = default;
  Pool(const char *name);

  TimerHeap & timer_heap();
  IOQueue   & io_queue();
  DNSResolv & dns_resolv();

  void set_stun(ip_addr_t ip_addr);

  ICETrans ice_trans(std::function<void(ICECall, std::string_view)> &&on_data_recv,
                     std::function<void(ice_trans_op_t, status_t)> &&on_ice_complete,
                     std::function<void(ICECall)> &&on_call_connect);


private:
  ice_trans_cfg_t _ice_cfg;

  pool_t _pool;

  TimerHeap _timer_heap;
  IOQueue   _io_queue;
  DNSResolv _dns_resolv;
};

}

#endif //T_MAN_POOL_H

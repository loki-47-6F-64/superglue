//
// Created by loki on 24-1-19.
//

#include "pool.h"

namespace pj {
caching_pool_t caching_pool;
pj_caching_pool caching_pool_raw;

Pool::Pool(const char *name) {
  std::once_flag flag;
  std::call_once(flag, []() {
    pj_caching_pool_init(&caching_pool_raw, nullptr, 0);
    caching_pool.reset(&caching_pool_raw);
  });

  pj_ice_strans_cfg_default(&_ice_cfg);

  _ice_cfg.af = pj_AF_INET();
  _ice_cfg.stun_cfg.pf = &caching_pool->factory;

  _pool.reset(pj_pool_create(&caching_pool->factory, "Loki-ICE", 512, 512, nullptr));

  _timer_heap = TimerHeap(_pool, _ice_cfg);
  _io_queue   = IOQueue(_pool, _ice_cfg);
  _dns_resolv = DNSResolv(_pool, _timer_heap, _io_queue);
}

TimerHeap &Pool::timer_heap() {
  return _timer_heap;
}

IOQueue &Pool::io_queue() {
  return _io_queue;
}

ICETrans Pool::ice_trans(std::function<void(ICECall, std::string_view)> &&on_data_recv,
                         std::function<void(ice_trans_op_t, status_t)> &&on_ice_complete,
                         std::function<void(ICECall)> &&on_call_connect) {
  return pj::ICETrans {
    _ice_cfg,
    std::make_unique<ICETrans::func_t::element_type>(std::make_tuple(std::move(on_data_recv), std::move(on_ice_complete), std::move(on_call_connect)))
  };
}

DNSResolv &Pool::dns_resolv() {
  return _dns_resolv;
}

void Pool::set_stun(ip_addr_t ip_addr) {
  if(ip_addr.port) {
    _ice_cfg.stun.port = (std::uint16_t) ip_addr.port;
  }

  _ice_cfg.stun.server = string(ip_addr.ip);
}
}
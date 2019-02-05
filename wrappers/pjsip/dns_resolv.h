//
// Created by loki on 27-1-19.
//

#ifndef T_MAN_DNSRESOLV_H
#define T_MAN_DNSRESOLV_H

#include "nath.h"
#include "io_queue.h"
#include "timer_heap.h"

namespace pj {
class DNSResolv {
public:
  DNSResolv() = default;

  DNSResolv(pool_t &pool, TimerHeap &timer_heap, IOQueue &io_queue);

  status_t set_ns(const std::vector<std::string_view> &servers);
private:
  dns_resolv_t _dns_resolv;
};
}

#endif //T_MAN_DNSRESOLV_H

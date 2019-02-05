//
// Created by loki on 27-1-19.
//

#include <kitty/util/set.h>
#include "dns_resolv.h"

namespace pj {

DNSResolv::DNSResolv(pool_t &pool, TimerHeap &timer_heap, IOQueue &io_queue) {
  pj_dns_resolver *resolv;

  pj_dns_resolver_create(pool->factory, nullptr, 0, timer_heap.raw(), io_queue.raw(), &resolv);
  _dns_resolv.reset(resolv);
}

status_t DNSResolv::set_ns(const std::vector<std::string_view> &_servers) {
  auto servers = util::map(_servers, [](const auto &server) { return string(server); });

  return pj_dns_resolver_set_ns(_dns_resolv.get(), servers.size(), servers.data(), nullptr);
}
}
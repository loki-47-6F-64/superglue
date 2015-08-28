#include <kitty/server/server.h>
#include <kitty/server/tcp_client.h>
#include <kitty/util/utility.h>
namespace server {
template<>
util::Optional<tcp::Client> tcp::_accept() {
  sockaddr_in6 client_addr;
  socklen_t addr_size {sizeof (client_addr)};

  int client_fd = accept(_listenfd.fd, (sockaddr *) & client_addr, &addr_size);

  if (client_fd < 0) {
    return {};
  }

  char ip_buf[INET6_ADDRSTRLEN];
  inet_ntop(AF_INET6, &client_addr.sin6_addr, ip_buf, INET6_ADDRSTRLEN);

  return Client {
    util::mk_uniq<file::io>(3000 * 1000, client_fd), {
      ip_buf
    }
  };
}

template<>
int tcp::_socket() {
  return socket(AF_INET6, SOCK_STREAM, 0);
}
};
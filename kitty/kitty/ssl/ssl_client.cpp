#include <kitty/ssl/ssl_client.h>
#include <kitty/ssl/ssl.h>
#include <kitty/util/utility.h>

namespace server {
template<>
util::Optional<ssl::Client> ssl::_accept() {
  sockaddr_in6 client_addr;
  socklen_t addr_size {sizeof (client_addr)};

  int client_fd = accept(_listenfd.fd, (sockaddr *) & client_addr, &addr_size);

  if (client_fd < 0) {
    return {};
  }

  char ip_buf[INET6_ADDRSTRLEN];
  inet_ntop(AF_INET6, &client_addr.sin6_addr, ip_buf, INET6_ADDRSTRLEN);


  file::ssl socket = ::ssl::accept(_member, client_fd);
  if(!socket.is_open()) {
    return {};
  }
  
  return Client {
    util::mk_uniq<file::ssl>(std::move(socket)),
    ip_buf
  };
}

template<>
int ssl::_socket() {
  return socket(AF_INET6, SOCK_STREAM, 0);
}
}
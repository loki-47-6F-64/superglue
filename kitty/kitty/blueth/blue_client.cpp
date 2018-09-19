#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include <kitty/blueth/blue_client.h>
#include <kitty/util/utility.h>

namespace server {
template<>
util::Optional<bluetooth::Client> bluetooth::_accept() {
  Client::_sockaddr client_addr;

  socklen_t addr_size = sizeof(client_addr);
  
  int client_fd = accept(_listenfd.fd, (sockaddr *) &client_addr, &addr_size);

  if (client_fd < 0) {
    return {};
  }

  auto dev = bt::device(client_addr.l2_bdaddr, 0);
  return Client {
    util::mk_uniq<file::blueth>(std::chrono::seconds(3), client_fd, *_member, dev),
    dev,
    23,
    nullptr,
    Client::LOW
  };
}

template<>
int bluetooth::_socket() {
  return socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
}
}
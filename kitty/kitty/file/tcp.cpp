#include <unistd.h>
#include <sys/socket.h>

#include <netdb.h>
#include <cstring>

#include <kitty/file/tcp.h>

namespace err {
extern void set(const char *err);
}

namespace file {
io connect(const char *hostname, const char *port) {
  constexpr long timeout = 0;
  
  addrinfo hints;
  addrinfo *server;
  
  std::memset(&hints, 0, sizeof (hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  
  if(int err = getaddrinfo(hostname, port, &hints, &server)) {
    err::set(gai_strerror(err));
    return {};
  }
  
  io sock { timeout, socket(AF_INET, SOCK_STREAM, 0) };
  
  if(connect(sock.getStream().fd(), server->ai_addr, server->ai_addrlen)) {
    freeaddrinfo(server);
    
    err::code = err::LIB_SYS;
    return {};
  }
  
  freeaddrinfo(server);
  
  return sock;
}
}

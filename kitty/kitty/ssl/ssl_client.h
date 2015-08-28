#ifndef SSL_CLIENT_H
#define SSL_CLIENT_H

#include <memory>

#include <arpa/inet.h>
#include <kitty/ssl/ssl_stream.h>
#include <kitty/ssl/ssl.h>
#include <kitty/server/server.h>
namespace server {
struct SslClient {
  typedef sockaddr_in6 _sockaddr;

  std::unique_ptr<file::ssl> socket;
  std::string ip_addr;
};

template<>
struct DefaultType<SslClient> {
  typedef ::ssl::Context Type;
};

typedef Server<SslClient> ssl;
}

#endif
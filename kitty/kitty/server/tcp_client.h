/* 
 * File:   client.h
 * Author: loki
 *
 * Created on June 10, 2014, 4:29 PM
 */

#ifndef TCP_CLIENT_H
#define	TCP_CLIENT_H

#include <memory>

#include <arpa/inet.h>

#include <kitty/file/io_stream.h>
#include <kitty/server/server.h>

namespace server {

struct TcpClient {
  typedef sockaddr_in6 _sockaddr;

  std::unique_ptr<file::io> socket;
  std::string ip_addr;
};

typedef Server<TcpClient> tcp;
}

#endif	/* CLIENT_H */


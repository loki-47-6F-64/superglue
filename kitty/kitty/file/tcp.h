#ifndef KITTY_TCP_H
#define KITTY_TCP_H

#include <kitty/file/io_stream.h>

namespace file {
  io connect(const char *hostname, const char *port);
}

#endif
#ifndef VIKING_BLUE_STREAM_H
#define VIKING_BLUE_STREAM_H

#include <kitty/file/io_stream.h>
#include <kitty/blueth/blueth.h>
namespace file {
namespace stream {
class blueth : public io {
  bt::HCI *_hci;
  bt::device *_dev;
  
public:
  blueth();
  blueth(int fd, bt::HCI& hci, bt::device& dev);
  
  void seal();
};
}

typedef FD<stream::blueth> blueth;
}

#endif

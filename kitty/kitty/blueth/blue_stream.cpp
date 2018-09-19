#include <kitty/blueth/blue_stream.h>

namespace file {
namespace stream {
blueth::blueth() : io() {}
blueth::blueth(int fd, bt::HCI& hci, bt::device& dev) : io(fd), _hci(&hci), _dev(&dev) {}

void blueth::seal() {
  io::seal();

  auto handle = _hci->getConnHandle(*_dev);
  if(handle) {
    _hci->disconnect(handle);
  }
}

}
}

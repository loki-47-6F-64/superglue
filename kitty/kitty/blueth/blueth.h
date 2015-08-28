#ifndef BDTRACKER_BLUETOOTH_H
#define BDTRACKER_BLUETOOTH_H

#include <vector>
#include <string>
#include <ctime>
#include <memory>

#include <bluetooth/bluetooth.h>

#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include <kitty/util/optional.h>
namespace bt {
class Uuid;
struct device;

typedef std::vector<device> device_arr;

struct device {
  bdaddr_t bdaddr;
  int rssi;

  device() = default;

  device(bdaddr_t bdaddr, int8_t rssi) :
  bdaddr(bdaddr), rssi(rssi) { }

  device(const char *bdaddr, int8_t rssi) :
  rssi(rssi) {
    str2ba(bdaddr, &this->bdaddr);
  }

  operator std::string() {
    std::string bdaddr;
    bdaddr.resize(18);

    ba2str(&this->bdaddr, (char*)bdaddr.c_str());

    return bdaddr;
  };
};

class HCI : public device {
  typedef util::Optional<uint16_t> optional;
  
  int _dev_id;
  int _hci_sock;
  
public:
  HCI();
  HCI(const char *bdaddr);
  HCI(HCI &&dev);

  ~HCI();

  void operator =(HCI &&dev);

  /*
   * Data broadcasted. The receiver doesn't need to create a connection.
   */
  int broadcast(uint8_t *data, uint8_t length);
  int broadcast(const std::vector< bt::Uuid > &uuids);
  
  /*
   * Start or stop advertising.
   * name could be a nullptr
   */
  int advertise(bool enable, const char *name);
  
  int disconnect(uint16_t handle);
  optional getConnHandle(device &dev);
  
private:
  
  int _setResponseData(uint8_t *data, uint8_t length);
};
}

#endif

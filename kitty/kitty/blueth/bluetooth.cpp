#include <algorithm>

#include <unistd.h>
#include <sys/poll.h>
#include <sys/ioctl.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include <kitty/blueth/blueth.h>
#include <kitty/blueth/ble.h>

#include <kitty/util/utility.h>
#include <kitty/err/err.h>
#include <kitty/blueth/uuid.h>

namespace bt {

constexpr uint8_t MAX_BROADCAST_DATA = 31;
HCI::HCI() : device() {
  _dev_id = hci_get_route(nullptr);

  _hci_sock = hci_open_dev(_dev_id);
  if (_dev_id < 0 || _hci_sock < 0) {
    err::code = err::LIB_SYS;
  }

  hci_devba(_dev_id, &this->bdaddr);
}

HCI::HCI(const char *bdaddr) : device(bdaddr, 0) {
  _dev_id = hci_get_route(&this->bdaddr);

  _hci_sock = hci_open_dev(_dev_id);
  if (_dev_id < 0 || _hci_sock < 0) {
    err::code = err::LIB_SYS;
  }
}

HCI::HCI(HCI && dev) noexcept {
  this->rssi = dev.rssi;
  this->bdaddr = dev.bdaddr;
  this->_dev_id = dev._dev_id;
  this->_hci_sock = dev._hci_sock;

  dev._dev_id = dev._hci_sock = -1;
}

struct conn_info {
  hci_conn_list_req req;
  
  // FIXME: Account for multiple connections
  hci_conn_info info[1];
};

HCI::optional HCI::getConnHandle(device &dev) {
  conn_info info;
  
  info.req.conn_num = 1;
  info.req.dev_id = _dev_id;
  

  if(ioctl(_hci_sock, HCIGETCONNLIST, &info) < 0) {
    err::code = err::LIB_SYS;
    
    return optional();
  }
  
  uint16_t handle;
  
  for(hci_conn_info &x: info.info) {
    if(bacmp(&x.bdaddr, &dev.bdaddr) == 0) {
      handle = x.handle;
      
      return optional(handle);
    }
  }
  
  return optional();  
}

HCI& HCI::operator =(HCI && dev) noexcept {
  std::swap(this->rssi,      dev.rssi);
  std::swap(this->bdaddr,    dev.bdaddr);
  std::swap(this->_dev_id,   dev._dev_id);
  std::swap(this->_hci_sock, dev._hci_sock);

  return *this;
}

HCI::~HCI() {
  if (_hci_sock != -1) {
    close(_hci_sock);
  }

  if (_dev_id != -1) {
    close(_dev_id);
  }
}


int HCI::scanResponse(const char *name) {
  uint8_t data_buf[256];

  uint8_t name_len = (uint8_t)strlen(name);
  uint8_t data_len = name_len + 2;

  data_buf[0] = name_len +1;
  data_buf[1] = 0x08; // Flag for name
  memcpy(&data_buf[2], name, name_len);

  return _setResponseData(data_buf, data_len);
}

int HCI::broadcast(const std::vector<Uuid> &uuids) {
  std::vector<Uuid> uuids16, uuids128;

  for (auto & uuid : uuids) {
    if (uuid.type == Uuid::BT_UUID16) {
      uuids16.push_back(uuid);
    }
    else {
      uuids128.push_back(uuid);
    }
  }

  std::vector<uint8_t> data;

  data.push_back(0x02);
  data.push_back(0x01);
  data.push_back(0x05);

  if (!uuids16.empty()) {
    const uint8_t data_len = 1 + 2 * uuids16.size();
    
    data.push_back(data_len);
    data.push_back(0x03);
    for (auto & uuid : uuids16) {
      util::append_struct(data, uuid.value.u16);
    }
  }

  if (!uuids128.empty()) {
    const uint8_t data_len = 1 + 16 * uuids128.size();
    
    data.push_back(data_len);
    data.push_back(0x06);
    for (auto & uuid : uuids128) {
      util::append_struct(data, uuid.value.u128);
    }
  }

  return broadcast(data.data(), data.size());
}

int HCI::ibeacon(const Uuid &uuid, uint16_t major, uint16_t minor, uint8_t txPower) {
  uint8_t data[30];

  data[0] = 0x02; // data length
  data[1] = 0x01; // data flags

  data[2] = 0x06; // LE and BR/EDR flag
  data[3] = 0x1a; // data length

  data[4] = 0xff; // ibeacon
  data[5] = 0x4c;
  data[6] = 0x00;
  data[7] = 0x02;
  data[8] = 0x15;

  *(uint128_t*)&data[9] = util::endian::big(uuid.value.u128);

  uint16_t *_data = (uint16_t*)(&data[25]);

  _data[0] = util::endian::big(major);
  _data[1] = util::endian::big(minor);

  data[29] = txPower;

  return broadcast(data, sizeof(data));
}

int HCI::advertisingEnable(uint16_t min_interval, uint16_t max_interval, bool enable) {

  le_set_advertising_parameters_cp adv_params_cp;
  memset(&adv_params_cp, 0, sizeof(adv_params_cp));
  adv_params_cp.min_interval = util::endian::little(min_interval);
  adv_params_cp.max_interval = util::endian::little(max_interval);
  adv_params_cp.chan_map = 7; // All adv channels
  adv_params_cp.advtype = 0;

  uint8_t status = 0;
  hci_request rq;
  memset(&rq, 0, sizeof(rq));

  rq.ogf = OGF_LE_CTL;
  rq.ocf = OCF_LE_SET_ADVERTISING_PARAMETERS;
  rq.cparam = &adv_params_cp;
  rq.clen = LE_SET_ADVERTISING_PARAMETERS_CP_SIZE;
  rq.rparam = &status;
  rq.rlen = 1;

  if (hci_send_req(_hci_sock, &rq, 1000) < 0) {
    err::code = err::LIB_SYS;

    return -1;
  }

  le_set_advertise_enable_cp advertise_cp;
  memset(&advertise_cp, 0, sizeof(advertise_cp));
  advertise_cp.enable = (uint8_t)(enable ? 0x01 : 0x00);

  memset(&rq, 0, sizeof(rq));
  rq.ogf = OGF_LE_CTL;
  rq.ocf = OCF_LE_SET_ADVERTISE_ENABLE;
  rq.cparam = &advertise_cp;
  rq.clen = LE_SET_ADVERTISE_ENABLE_CP_SIZE;
  rq.rparam = &status;
  rq.rlen = 1;

  if (hci_send_req(_hci_sock, &rq, 1000) < 0) {
    err::code = err::LIB_SYS;
    return -1;
  }

  return 0;
}

int HCI::broadcast(uint8_t *data, uint8_t length) {
  if (length > MAX_BROADCAST_DATA) {
    err::code = err::OUT_OF_BOUNDS;
    return -1;
  }


  hci_request rq;
  le_set_advertising_data_cp data_cp;
  uint8_t status = 12;

  memset(&data_cp, 0, sizeof(data_cp));
  data_cp.length = length;
  memcpy(&data_cp.data, data, sizeof(data_cp.data));

  memset(&rq, 0, sizeof(rq));
  rq.ogf = OGF_LE_CTL;
  rq.ocf = OCF_LE_SET_ADVERTISING_DATA;
  rq.cparam = &data_cp;
  rq.clen = LE_SET_ADVERTISING_DATA_CP_SIZE;
  rq.rparam = &status;
  rq.rlen = 1;

  if (hci_send_req(_hci_sock, &rq, 1000) < 0) {
    err::code = err::LIB_SYS;
    return -1;
  }

  if (status) {
    err::code = err::INPUT_OUTPUT;
    return -1;
  }

  return 0;
}

int HCI::_setResponseData(uint8_t *data, uint8_t length) {
  hci_request rq;
  le_set_scan_response_data_cp data_cp;
  uint8_t status = 0;

  memset(&data_cp, 0, sizeof(data_cp));
  data_cp.length = length;
  memcpy(&data_cp.data, data, sizeof(data_cp.data));

  memset(&rq, 0, sizeof(rq));
  rq.ogf = OGF_LE_CTL;
  rq.ocf = OCF_LE_SET_SCAN_RESPONSE_DATA;
  rq.cparam = &data_cp;
  rq.clen = LE_SET_SCAN_RESPONSE_DATA_CP_SIZE;
  rq.rparam = &status;
  rq.rlen = 1;

  if (hci_send_req(_hci_sock, &rq, 1000) < 0) {
    err::code = err::LIB_SYS;
    return -1;
  }

  if (status) {
    err::code = err::INPUT_OUTPUT;
    return -1;
  }

  return 0;
}

int HCI::disconnect(uint16_t handle) {
  if(hci_disconnect(_hci_sock, handle, HCI_OE_USER_ENDED_CONNECTION, 1000)) {
    err::code = err::LIB_SYS;
    
    return -1;
  }
  
  return 0;
}

}





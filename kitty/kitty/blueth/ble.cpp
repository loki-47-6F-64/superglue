#include <tuple>
#include <unistd.h>

#include "kitty/blueth/ble.h"
#include "kitty/log/log.h"
#include "kitty/server/server.h"
namespace bt {
Uuid::Uuid(const char *uuid) {
  bt_string_to_uuid(this, uuid);
}

Descriptor::Descriptor(
  const char *uuid,
  std::vector<uint8_t> && data,
  props_t properties, props_t secure)

  : uuid(uuid), properties(properties), secure(secure), data(std::move(data)) { }

Characteristic::Characteristic(
  const char *uuid,
  std::vector<uint8_t> && data,
  read_func_type  && readCallback,
  write_func_type && writeCallback,
  std::vector<Descriptor> && descriptors,
  props_t properties, props_t secure)

  : properties(properties),
    secure(secure),
    uuid(uuid),
    readCallback(std::move(readCallback)),
    writeCallback(std::move(writeCallback)),
    descriptors(std::move(descriptors)),
    data(std::move(data)) { }

bool Characteristic::secureRead() {
  return secure & READ;
}

bool Characteristic::secureWrite() {
  return secure & WRITE;
}

bool Characteristic::canRead() {
  return properties & READ;
}

bool Characteristic::canWrite() {
  return properties & WRITE;
}

bool Characteristic::canWriteWithoutResponse() {
  return properties & WRITE_WITHOUT_RESPONSE;
}

bool Characteristic::canNotify() {
  return properties & NOTIFY;
}

Service::Service(
  const char *uuid, std::vector< bt::Characteristic > && characteristics)
  : uuid(uuid), characteristics(std::move(characteristics)) { }

Profile::Profile(std::vector<Service> && services) : _services(std::move(services)) {
  uint16_t handle_p = 0x0000;
  for (auto & service : _services) {

    Handle handle;
    handle.type = Handle::SERVICE;
    handle.service = &service;
    service.startHandle = ++handle_p;

    _handles.push_back(handle);

    auto &characteristics = service.characteristics;
    for (auto & characteristic : characteristics) {

      Handle handle;
      handle.type = Handle::CHARACTERISTIC;
      handle.characteristic = &characteristic;
      characteristic.startHandle = ++handle_p;
      characteristic.valueHandle = ++handle_p;

      Handle placeholder;
      placeholder.type = Handle::CHARACTERISTICVALUE;

      _handles.push_back(handle);
      _handles.push_back(placeholder);

      // FIXME: Add descriptor for notifications


      for (auto & descriptor : characteristic.descriptors) {
        Handle handle;
        handle.type = Handle::DESCRIPTOR;
        handle.descriptor = &descriptor;
        descriptor.handle = ++handle_p;

        _handles.push_back(handle);
      }
    }

    service.endHandle = handle_p;
  }
}
}
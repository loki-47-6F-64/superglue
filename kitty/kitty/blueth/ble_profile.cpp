#include <kitty/blueth/ble.h>
#include <kitty/blueth/blue_client.h>
#include <kitty/util/utility.h>
#include <kitty/log/log.h>

namespace bt {

constexpr int ATT_OP_ERROR                    = 0x01;
constexpr int ATT_OP_MTU_REQ                  = 0x02;
constexpr int ATT_OP_MTU_RESP                 = 0x03;
constexpr int ATT_OP_FIND_INFO_REQ            = 0x04;
constexpr int ATT_OP_FIND_INFO_RESP           = 0x05;
constexpr int ATT_OP_FIND_BY_TYPE_REQ         = 0x06;
constexpr int ATT_OP_FIND_BY_TYPE_RESP        = 0x07;
constexpr int ATT_OP_READ_BY_TYPE_REQ         = 0x08;
constexpr int ATT_OP_READ_BY_TYPE_RESP        = 0x09;
constexpr int ATT_OP_READ_REQ                 = 0x0a;
constexpr int ATT_OP_READ_RESP                = 0x0b;
constexpr int ATT_OP_READ_BLOB_REQ            = 0x0c;
constexpr int ATT_OP_READ_BLOB_RESP           = 0x0d;
// constexpr int ATT_OP_READ_MULTI_REQ           = 0x0e;
// constexpr int ATT_OP_READ_MULTI_RESP          = 0x0f;
constexpr int ATT_OP_READ_BY_GROUP_REQ        = 0x10;
constexpr int ATT_OP_READ_BY_GROUP_RESP       = 0x11;
constexpr int ATT_OP_WRITE_REQ                = 0x12;
constexpr int ATT_OP_WRITE_RESP               = 0x13;
constexpr int ATT_OP_WRITE_CMD                = 0x52;
// constexpr int ATT_OP_PREP_WRITE_REQ           = 0x16;
// constexpr int ATT_OP_PREP_WRITE_RESP          = 0x17;
// constexpr int ATT_OP_EXEC_WRITE_REQ           = 0x18;
// constexpr int ATT_OP_EXEC_WRITE_RESP          = 0x19;
// constexpr int ATT_OP_HANDLE_NOTIFY            = 0x1b;
// constexpr int ATT_OP_HANDLE_IND               = 0x1d;
// constexpr int ATT_OP_HANDLE_CNF               = 0x1e;
// constexpr int ATT_OP_SIGNED_WRITE_CMD         = 0xd2;


constexpr int GATT_PRIM_SVC_UUID              = 0x2800;
// constexpr int GATT_INCLUDE_UUID               = 0x2802;
constexpr int GATT_CHARAC_UUID                = 0x2803;


// constexpr int GATT_CLIENT_CHARAC_CFG_UUID     = 0x2902;
// constexpr int GATT_SERVER_CHARAC_CFG_UUID     = 0x2903;


// constexpr int ATT_ECODE_SUCCESS               = 0x00;
constexpr int ATT_ECODE_INVALID_HANDLE        = 0x01;
constexpr int ATT_ECODE_READ_NOT_PERM         = 0x02;
constexpr int ATT_ECODE_WRITE_NOT_PERM        = 0x03;
// constexpr int ATT_ECODE_INVALID_PDU           = 0x04;
constexpr int ATT_ECODE_AUTHENTICATION        = 0x05;
constexpr int ATT_ECODE_REQ_NOT_SUPP          = 0x06;
constexpr int ATT_ECODE_INVALID_OFFSET        = 0x07;
// constexpr int ATT_ECODE_AUTHORIZATION         = 0x08;
// constexpr int ATT_ECODE_PREP_QUEUE_FULL       = 0x09;
constexpr int ATT_ECODE_ATTR_NOT_FOUND        = 0x0a;
// constexpr int ATT_ECODE_ATTR_NOT_LONG         = 0x0b;
// constexpr int ATT_ECODE_INSUFF_ENCR_KEY_SIZE  = 0x0c;
// constexpr int ATT_ECODE_INVAL_ATTR_VALUE_LEN  = 0x0d;
// constexpr int ATT_ECODE_UNLIKELY              = 0x0e;
// constexpr int ATT_ECODE_INSUFF_ENC            = 0x0f;
constexpr int ATT_ECODE_UNSUPP_GRP_TYPE       = 0x10;

struct Error {
  uint8_t  attribute_code;
  uint8_t  requestType;
  uint16_t handle;
  uint8_t  code;
} __attribute__((packed));

struct GenericReq {
  uint16_t startHandle;
  uint16_t endHandle;
} __attribute__((packed));

struct ReadByGroupReq : GenericReq {
  uint16_t uuid;
} __attribute__((packed));

struct FindByTypeReq : GenericReq {
  uint16_t type_uuid;
} __attribute__((packed));

struct ReadByTypeReq : GenericReq {
} __attribute__((packed));

struct FindInfoReq : GenericReq {
} __attribute__((packed));

std::vector<uint8_t> parseError(uint8_t requestType, uint16_t handle, uint8_t code) {
  std::vector<uint8_t> buf;
  Error error {
    ATT_OP_ERROR,
    requestType,
    handle,
    code
  };

  util::append_struct(buf, error);

  return buf;
}

std::vector<uint8_t> slice_data(std::vector<uint8_t> && data, uint16_t mtu, uint16_t start = 0, uint16_t offset = 0) {
  int compensation = 0;

  // When slice_data is called, offset < data.size()
  if (data.size() - offset > mtu) {
    compensation = data.size() - offset - mtu;
  }

  data.erase(data.end() - compensation, data.end());
  data.erase(data.begin() + start, data.begin() + start + offset);

  return data;
}

std::vector<uint8_t> Profile::_readByGroup(server::BlueClient &client) const {
  DEBUG_LOG("Executing ReadByGroup request.");

  std::vector<uint8_t> response;

  auto request = util::read_struct<ReadByGroupReq>(*client.socket);

  if (!request) {
    return response;
  }

  ReadByGroupReq &req = request;

  if (req.uuid != GATT_PRIM_SVC_UUID) {
    return parseError(ATT_OP_READ_BY_GROUP_REQ, req.startHandle, ATT_ECODE_UNSUPP_GRP_TYPE);
  }

  uint x;
  for (x = req.startHandle - 1; x < req.endHandle && x < _handles.size(); ++x) {
    if (_handles[x].type == Handle::SERVICE) {
      goto found_service;
    }
  }

// not_found_service:
  return parseError(ATT_OP_READ_BY_GROUP_REQ, req.startHandle, ATT_ECODE_ATTR_NOT_FOUND);

found_service:
  const int type = _handles[x].service->uuid.type;

  const uint8_t len_per_data = 4 + type / 8;

  const size_t max_data = (client.mtu - 2) / len_per_data;
  response.push_back(ATT_OP_READ_BY_GROUP_RESP);
  response.push_back(len_per_data);

  auto it  = _services.cbegin() + (_handles[x].service - _services.data());
  auto end = it + max_data;
  for (; it < end && it < _services.cend(); ++it) {
    if (type != it->uuid.type) {
      break;
    }

    util::append_struct(response, it->startHandle);
    util::append_struct(response, it->endHandle);

    if (type == Uuid::BT_UUID16) {
      util::append_struct(response, it->uuid.value.u16);
    }
    else {
      util::append_struct(response, it->uuid.value.u128);
    }

    DEBUG_LOG("startHandle :", it->startHandle, ": endHandle :", it->endHandle);
  }

  return response;
}

std::vector<uint8_t> Profile::_findByType(server::BlueClient &client) const {
  DEBUG_LOG("Executing FindByType request.");

  std::vector<uint8_t> response;
  auto request = util::read_struct<FindByTypeReq>(*client.socket);

  if (!request) {
    return response;
  }

  FindByTypeReq &req = request;
  if (req.type_uuid != GATT_PRIM_SVC_UUID) {
    return parseError(ATT_OP_FIND_BY_TYPE_REQ, req.startHandle, ATT_ECODE_ATTR_NOT_FOUND);
  }

  Uuid uuid;
  // The rest of the request should already be cached
  if (client.socket->getCache().size() == sizeof(FindByTypeReq) + 2 + 1) {
    uuid.type = Uuid::BT_UUID16;
    uuid.value.u16 = util::read_struct<decltype(uuid.value.u16)>(*client.socket);
  }
  else {
    uuid.type = Uuid::BT_UUID128;
    uuid.value.u128 = util::read_struct<decltype(uuid.value.u128)>(*client.socket);
  }

  std::vector<std::pair<uint16_t, uint16_t>> handles;
  for (uint x = req.startHandle - 1; x < req.endHandle && x < _handles.size(); ++x) {
    Service *const service = _handles[x].service;
    if (_handles[x].type == Handle::SERVICE && !uuid.compare(service->uuid)) {
      handles.emplace_back(service->startHandle, service->endHandle);
    }
  }

  if (handles.empty()) {
    return parseError(ATT_OP_FIND_BY_TYPE_REQ, req.startHandle, ATT_ECODE_ATTR_NOT_FOUND);
  }

  const size_t max_data = (client.mtu - 1) / 4;
  if (max_data < handles.size()) {
    handles.resize(max_data);
  }

  response.push_back(ATT_OP_FIND_BY_TYPE_RESP);
  for (auto & handle : handles) {
    DEBUG_LOG("startHandle: ", handle.first, " : endHandle: ", handle.second);

    util::append_struct(response, handle.first);
    util::append_struct(response, handle.second);
  }

  return response;
}

std::vector<uint8_t> Profile::_readByTypeMeta(server::BlueClient &client, bt::ReadByTypeReq &req) const {
  std::vector<uint8_t> response;
  uint x;
  for (x = req.startHandle - 1; x < req.endHandle && x < _handles.size(); ++x) {
    if (_handles[x].type == Handle::CHARACTERISTIC) {
      goto found_service;
    }
  }

// not_found_service:
  return parseError(ATT_OP_READ_BY_TYPE_REQ, req.startHandle, ATT_ECODE_ATTR_NOT_FOUND);

found_service:
  const uint8_t type = _handles[x].characteristic->uuid.type;

  // uint16_t *2 + byte + sizeof(type)
  const uint8_t len_per_data = 2 + 2 + 1 + type / 8;
  const size_t max_data = (client.mtu - 2) / len_per_data;

  response.push_back(ATT_OP_READ_BY_TYPE_RESP);
  response.push_back(len_per_data);

  uint num_data = 0;
  for (; x < req.endHandle && x < _handles.size(); ++x) {
    if (_handles[x].type == Handle::CHARACTERISTIC) {
      if (++num_data > max_data || _handles[x].characteristic->uuid.type != type) {
        break;
      }

      Characteristic &characteristic = *_handles[x].characteristic;
      util::append_struct(response, characteristic.startHandle);
      util::append_struct(response, characteristic.properties);
      util::append_struct(response, characteristic.valueHandle);

      if (type == Uuid::BT_UUID16) {
        util::append_struct(response, characteristic.uuid.value.u16);
      }
      else {
        util::append_struct(response, characteristic.uuid.value.u128);
      }
    }
  }

  return response;
}

std::vector<uint8_t> Profile::_readByType(server::BlueClient &client) const {
  DEBUG_LOG("Executing ReadByType request.");

  std::vector<uint8_t> response;
  auto request = util::read_struct<ReadByTypeReq>(*client.socket);

  if (!request) {
    return response;
  }

  ReadByTypeReq &req = request;

  Uuid uuid;
  // The rest of the request should already be cached
  if (client.socket->getCache().size() == sizeof(ReadByTypeReq) + 2 + 1) {
    uuid.type = Uuid::BT_UUID16;
    uuid.value.u16 = util::read_struct<decltype(uuid.value.u16)>(*client.socket);
  }
  else {
    uuid.type = Uuid::BT_UUID128;
    uuid.value.u128 = util::read_struct<decltype(uuid.value.u128)>(*client.socket);
  }


  if (uuid.type == Uuid::BT_UUID16 && uuid.value.u16 == GATT_CHARAC_UUID) {
    return _readByTypeMeta(client, req);
  }

  else {
    uint x;
    for (x = req.startHandle - 1; x < req.endHandle && x < _handles.size(); ++x) {
      if ((_handles[x].type == Handle::CHARACTERISTIC && !uuid.compare(_handles[x].characteristic->uuid))
          ||
          (_handles[x].type == Handle::DESCRIPTOR && !uuid.compare(_handles[x].descriptor->uuid))
         ) {
        goto found_service;
      }
    }

// not_found_service:
    return parseError(ATT_OP_READ_BY_TYPE_REQ, req.startHandle, ATT_ECODE_ATTR_NOT_FOUND);

  found_service:

    std::vector<uint8_t> *data;
    uint16_t handle;
    if (_handles[x].type == Handle::CHARACTERISTIC) {
      Characteristic &characteristic = *_handles[x].characteristic;

      if (characteristic.secureRead() &&
          client.security < server::BlueClient::MEDIUM) {

        return parseError(ATT_OP_READ_BY_TYPE_REQ, req.startHandle, ATT_ECODE_AUTHENTICATION);
      }
      data = &characteristic.data;
      handle = characteristic.valueHandle;
    }
    else {
      Descriptor &descriptor = *_handles[x].descriptor;

      data = &descriptor.data;
      handle = descriptor.handle;
    }
    uint8_t data_len;
    if (data->size() + 4 > client.mtu) {
      data_len = client.mtu - 2;
    }
    else {
      data_len = data->size() + 2;
    }

    response.push_back(ATT_OP_READ_BY_TYPE_RESP);
    response.push_back(data_len);
    util::append_struct(response, handle);

    response.insert(response.end(), data->cbegin(), data->cend());

    return slice_data(std::move(response), client.mtu);
  }
}

std::vector<uint8_t> Profile::_findInfo(server::BlueClient &client) const {
  DEBUG_LOG("Executing FindInfo request.");

  std::vector<uint8_t> response;
  auto request = util::read_struct<FindInfoReq>(*client.socket);

  if (!request) {
    return response;
  }

  FindInfoReq &req = request;

  if (req.startHandle > _handles.size()) {
    return parseError(ATT_OP_FIND_INFO_REQ, req.startHandle, ATT_ECODE_ATTR_NOT_FOUND);
  }

  std::vector<Uuid> uuids;
  for (uint x = req.startHandle - 1; x < req.endHandle && x < _handles.size(); ++x) {
    switch (_handles[x].type) {
    case Handle::SERVICE:
      print(debug, "Found service");
      uuids.emplace_back("2800");
      break;
    case Handle::CHARACTERISTIC:
      print(debug, "Found characteristic");
      uuids.emplace_back("2803");
      break;
    case Handle::CHARACTERISTICVALUE:
      print(debug, "Found characteristic value");
      uuids.push_back(_handles[x - 1].characteristic->uuid);
      break;
    case Handle::DESCRIPTOR:
      print(debug, "Found descriptor");
      uuids.push_back(_handles[x].descriptor->uuid);
      break;
    }
  }

  const uint16_t type = uuids.front().type;
  const uint8_t data_len = 2 + type / 8;
  const size_t max_data = (client.mtu - 2) / data_len;

  const size_t num_data = uuids.size() > max_data ? max_data : uuids.size();

  response.push_back(ATT_OP_FIND_INFO_RESP);
  response.push_back(type == Uuid::BT_UUID16 ? 0x01 : 0x02);


  for (uint x = 0; x < num_data; ++x) {
    if (type != uuids[x].type) {
      break;
    }

    const uint16_t handle = x + req.startHandle;
    util::append_struct(response, handle);

    if (type == Uuid::BT_UUID16) {
      util::append_struct(response, uuids[x].value.u16);
    }
    else {
      util::append_struct(response, uuids[x].value.u128);
    }
  }

  return response;
}

std::vector<uint8_t> Profile::_read(server::BlueClient &client, uint8_t requestType) const {
  DEBUG_LOG("Executing read request.");

  std::vector<uint8_t> response;

  auto request = util::read_struct<uint16_t>(*client.socket);
  if (!request) {
    return response;
  }


  uint16_t handle = request - 1;
  if (handle >= _handles.size()) {
    return parseError(requestType, handle + 1, ATT_ECODE_INVALID_HANDLE);
  }

  uint16_t offset = 0;
  if (requestType == ATT_OP_READ_BLOB_REQ) {
    auto optional_offset = util::read_struct<uint16_t>(*client.socket);
    if (!optional_offset) {
      return response;
    }

    offset = optional_offset;
  }

  response.push_back(requestType == ATT_OP_READ_REQ ? ATT_OP_READ_RESP : ATT_OP_READ_BLOB_RESP);

  const uint16_t type = _handles[handle].type;
  if (type == Handle::SERVICE) {
    Service &service = *_handles[handle].service;

    if (service.uuid.type == Uuid::BT_UUID16) {
      util::append_struct(response, service.uuid.value.u16);
    }
    else {
      util::append_struct(response, service.uuid.value.u128);
    }

    return slice_data(std::move(response), client.mtu, sizeof(requestType), offset);
  }

  if (type == Handle::CHARACTERISTIC) {
    Characteristic &characteristic = *_handles[handle].characteristic;

    response.push_back(characteristic.properties);
    util::append_struct(response, characteristic.valueHandle);

    if (characteristic.uuid.type == Uuid::BT_UUID16) {
      util::append_struct(response, characteristic.uuid.value.u16);
    }
    else {
      util::append_struct(response, characteristic.uuid.value.u128);
    }

    return slice_data(std::move(response), client.mtu, sizeof(requestType), offset);
  }

  if (type == Handle::CHARACTERISTICVALUE || type == Handle::DESCRIPTOR) {
    uint8_t secure;
    uint8_t properties;

    std::vector<uint8_t> *data;
    int x = handle;
    if (type == Handle::DESCRIPTOR) {
      Descriptor &descriptor = *_handles[handle].descriptor;
      secure     = descriptor.secure;
      properties = descriptor.properties;

      data = &descriptor.data;
      // Goto previous CHARACTERISTICVALUE
      while (_handles[--x].type != Handle::CHARACTERISTICVALUE);
    }
    else {
      Characteristic &characteristic = *_handles[handle - 1].characteristic;
      secure     = characteristic.secure;
      properties = characteristic.properties;

      data = &characteristic.data;
    }

    Characteristic &characteristic = *_handles[x - 1].characteristic;
    if (!(properties & READ)) {
      print(debug, "properties: ", properties);
      return parseError(requestType, handle + 1, ATT_ECODE_READ_NOT_PERM);
    }

    if (secure & READ && client.security < server::BlueClient::MEDIUM) {
      return parseError(requestType, handle + 1, ATT_ECODE_AUTHENTICATION);
    }

    if (data->empty()) {
      util::Optional<std::vector<uint8_t>> optional = characteristic.readCallback(offset, client.mtu - 1, client.session);

      if (!optional) {
        return parseError(requestType, handle + 1, ATT_ECODE_INVALID_OFFSET);
      }

      std::vector<uint8_t> &buf = optional;
      response.insert(response.end(), buf.cbegin(), buf.cend());
    }
    else {
      if (offset > data->size()) {
        return parseError(requestType, handle + 1, ATT_ECODE_INVALID_OFFSET);
      }

      response.insert(response.end(), data->cbegin(), data->cend());
      response = slice_data(std::move(response), client.mtu, sizeof(requestType), offset);
    }

    return response;
  }

  return response;
}

std::vector<uint8_t> Profile::_write(server::BlueClient &client, uint8_t requestType) const {
  DEBUG_LOG("Executing write request.");

  std::vector<uint8_t> response;
  auto handle = util::read_struct<uint16_t>(*client.socket);

  const bool noResponse = requestType == ATT_OP_WRITE_CMD;
  if (!handle) {
    return response;
  }

  --handle;
  if (handle >= _handles.size()) {
    return parseError(requestType, handle, ATT_ECODE_INVALID_HANDLE);
  }

  uint16_t properties = 0;
  uint16_t secure = 0;

  Characteristic *characteristic;
  switch (_handles[handle].type) {
  case Handle::SERVICE:
    break;
  case Handle::CHARACTERISTIC:
    characteristic = _handles[handle].characteristic;
    properties = characteristic->properties;
    secure = characteristic->secure;
    break;
  case Handle::CHARACTERISTICVALUE:
    characteristic = _handles[handle - 1].characteristic;
    properties = characteristic->properties;
    secure = characteristic->secure;
    break;
  case Handle::DESCRIPTOR:
    // FIXME: Handle notifications.
    break;
  }

  if (!(properties & (noResponse ? WRITE_WITHOUT_RESPONSE : WRITE))) {
    return parseError(requestType, handle, ATT_ECODE_WRITE_NOT_PERM);
  }

  if (secure & (noResponse ? WRITE_WITHOUT_RESPONSE : WRITE) &&
      client.security < server::BlueClient::MEDIUM
     ) {

    return parseError(requestType, handle, ATT_ECODE_AUTHENTICATION);
  }

  std::vector<uint8_t> &cache = client.socket->getCache();
  int result = characteristic->writeCallback(
    { cache.cbegin() + sizeof(uint16_t) + 1, cache.cend() },
    client.session
  );

  if (!noResponse) {
    if (result) {
      return parseError(requestType, handle, result);
    }

    response.push_back(ATT_OP_WRITE_RESP);
  }

  return response;
}

std::vector<uint8_t> Profile::_mtu(server::BlueClient &client) const {
  std::vector<uint8_t> response;

  auto mtu = util::read_struct<uint16_t>(*client.socket);
  if (!mtu) {
    return response;
  }

  if (mtu < 0x002a) {
    return parseError(ATT_OP_MTU_REQ, 0x0000, ATT_ECODE_REQ_NOT_SUPP);
  }

  client.mtu = mtu;

  DEBUG_LOG("Exchange mtu: ", client.mtu);

  response.push_back(ATT_OP_MTU_RESP);
  util::append_struct(response, client.mtu);

  return response;
}


void print_request(uint8_t requestType, std::vector<uint8_t> &request) {
  std::string req_str { "Request: " };
  for (auto & ch : request) {
    for (auto & hex_ch : util::hex(ch)) {
      req_str.push_back(hex_ch);
    }
    req_str.push_back(' ');
  }

  DEBUG_LOG(req_str);
}

void print_response(std::vector<uint8_t> &response) {
  std::string resp_str { "Response: " };
  for (auto & ch : response) {
    for (auto & hex_ch : util::hex(ch)) {
      resp_str.push_back(hex_ch);
    }
    resp_str.push_back(' ');
  }

  DEBUG_LOG(resp_str);
}

int Profile::main(server::BlueClient &client) const {
  std::vector<uint8_t> response;

  while (!client.socket->eof()) {
    util::Optional<uint8_t> requestType = client.socket->next();

    if (!requestType) {
      if(err::code == err::TIMEOUT) {
	client.socket->seal();
      }
      
      break;
    }

    print_request(requestType, client.socket->getCache());
    switch (requestType) {
    case ATT_OP_READ_BY_GROUP_REQ:
      response = _readByGroup(client);
      break;
    case ATT_OP_FIND_BY_TYPE_REQ:
      response = _findByType(client);
      break;
    case ATT_OP_READ_BY_TYPE_REQ:
      response = _readByType(client);
      break;
    case ATT_OP_FIND_INFO_REQ:
      response = _findInfo(client);
      break;

      // FIXME: Possible error: gattool whines about protocol error
    case ATT_OP_READ_BLOB_REQ:
      response = _read(client, ATT_OP_READ_BLOB_REQ);
      break;
    case ATT_OP_READ_REQ:
      response = _read(client, ATT_OP_READ_REQ);
      break;
    case ATT_OP_WRITE_REQ:
      response = _write(client, ATT_OP_WRITE_REQ);
      break;
    case ATT_OP_WRITE_CMD:
      _write(client, ATT_OP_WRITE_CMD);
      break;
    case ATT_OP_MTU_REQ:
      response = _mtu(client);
      break;
    default:
      response = parseError(requestType, 0x0000, ATT_ECODE_REQ_NOT_SUPP);
      print(error, "Request: 0x", util::hex(requestType), " not supported");
      break;
    }

    print_response(response);

    print(*client.socket, response);
  }

  return !client.socket->eof();
}
}

//
// Created by loki on 3-8-15.
//

#include <kitty/err/err.h>
#include <sodium.hpp>
#include <config.hpp>
#include <kitty/util/utility.h>


namespace sodium {
static const Json::Value *find(const Json::Value &val, const char *begin)   {
  auto end = begin + strlen(begin);

  return val.find(begin, end);
}

util::Optional<std::string> Session::unpack(const std::string &mesg) {
  // Uncommenting this piece of code seams to cause a segfault when compiled with gcc for android
  // TODO: find out why.
  //  logManager->log(gen::log_severity::DEBUG, mesg);


  Json::Value pack;
  Json::Reader reader;

  if(!reader.parse(mesg, pack)) {
    err::set(reader.getFormattedErrorMessages());

    return {};
  }

  auto public_it       = find(pack, "public_key");
  auto nonce_it        = find(pack, "nonce");
  auto payload_it      = find(pack, "payload");
  auto payload_size_it = find(pack, "payload_size");

  if(!public_it) {
    err::set("Missing parameter [public_key]");
    return {};
  }
  if(!nonce_it) {
    err::set("Missing parameter [nonce]");
    return {};
  }
  if(!payload_it) {
    err::set("Missing parameter [payload]");
    return {};
  }
  if(!payload_size_it) {
    err::set("Missing parameter [payload_size]");
    return {};
  }

  auto pub_hex          = public_it->asCString();
  auto nonce_hex        = nonce_it->asCString();
  auto payload_hex      = payload_it->asCString();
  uint32_t payload_size = payload_size_it->asUInt();

  pubKey key;
  nonce n;

  auto payload = from_hex<char>(util::toContainer(payload_hex));

  if(!payload ||
     from_hex(util::toContainer(pub_hex), key) ||
     from_hex(util::toContainer(nonce_hex), n)) {
    return {};
  }

  std::vector<char> unpacked;
  unpacked.resize(payload_size +1);

  if(decrypt(*payload, unpacked, n, key, _local.private_)) {
    return {};
  }

  // Omit '\0'
  return { std::string(std::begin(unpacked), std::end(unpacked) -1) };

}
}

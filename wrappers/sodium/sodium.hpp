//
// Created by loki on 3-8-15.
//

#pragma once


#include <sodium.h>
#include <array>
#include <vector>

#include <json/json.h>
#include <kitty/util/optional.h>
#include <kitty/util/set.h>

namespace sodium {

// std::array() lacks a constructor for iterators
template<class T, std::size_t N>
class array : public std::array<T, N> {
public:
  array() = default;
  array(const array &) = default;
  array(array &&) = default;

  array &operator=(const array &) = default;
  array &operator=(array &&) = default;

  template<class It>
  array(It begin, It end) {
    std::copy(begin, end, std::begin(*this));
  }
};

template<class Container>
uint64_t capacity(const Container& c) {
  return std::distance(std::begin(c), std::end(c));
}

constexpr auto MACBytes = crypto_box_MACBYTES;

template<std::size_t length>
using msg = array<uint8_t, MACBytes + length>;
using nonce = array<uint8_t, crypto_box_NONCEBYTES>;

using pubKey  = array<uint8_t, crypto_box_PUBLICKEYBYTES>;
using privKey = array<uint8_t, crypto_box_SECRETKEYBYTES>;

template<class Container>
std::vector<char> to_hex(const Container &bin) {
  constexpr auto element_size = sizeof(decltype(*std::begin(bin)));

  std::vector<char> bin_hex;

  bin_hex.resize(capacity(bin) * element_size * 2 +1);
  sodium_bin2hex(bin_hex.data(), capacity(bin_hex), (uint8_t*)bin.data(), capacity(bin));

  return bin_hex;
}

template<class ContainerIn, class ContainerOut>
int from_hex(const ContainerIn &hex, ContainerOut &bin) {
  typedef decltype(*std::begin(bin)) element_t;

  std::size_t bin_len;
  const char *hex_end;
  return sodium_hex2bin((uint8_t*)bin.data(), capacity(bin) * sizeof(element_t), hex.data(), capacity(hex), nullptr, &bin_len, &hex_end);
}

template<class T, class Container>
util::Optional<std::vector<T>> from_hex(const Container &hex) {
  typedef T element_t;

  constexpr std::size_t hex_size = sizeof(element_t) *2;

  auto size = capacity(hex);

  // A string won't show it's internal '\0'
  if(size % hex_size == 0) {
    ++size;
  }

  if(size % hex_size != 1) {
    return {};
  }

  std::vector<element_t> bin;
  bin.resize((size -1) / hex_size);

  if(from_hex(hex, bin)) {
    return {};
  }

  return std::move(bin);
};


class KeyPair {
public:
  pubKey  public_;
  privKey private_;

  static const KeyPair generate() {
    KeyPair pair;

    crypto_box_keypair(pair.public_.data(), pair.private_.data());

    return pair;
  }
};

template<class ContainerIn, class ContainerOut>
int encrypt(const ContainerIn &in, ContainerOut &out, nonce& n, const pubKey &pub, const privKey &priv) {
  if(capacity(out) < MACBytes + capacity(in)) {
    return -1;
  }

  // uint8_t nonce[crypto_box_NONCEBYTES];
  randombytes_buf(n.data(), capacity(n));

  crypto_box_easy((uint8_t*)out.data(), (uint8_t*)in.data(), capacity(in), n.data(), pub.data(), priv.data());

  return 0;
}

template<class ContainerIn, class ContainerOut>
int decrypt(const ContainerIn &in, ContainerOut &out, nonce& n, const pubKey &pub, const privKey &priv) {
  // Protect against underflow
  if(capacity(in) < MACBytes || capacity(out) < capacity(in) - MACBytes) {
    err::set("Container capacity too small");
    return -1;
  }

  if(crypto_box_open_easy((uint8_t*)out.data(), (uint8_t*)in.data(), capacity(in), n.data(), pub.data(), priv.data())) {
    err::set("Authentication failiure");
    return -1;
  }

  return 0;
}

class Session {
  const KeyPair _local;
  const pubKey _remote;
public:

  Session(const KeyPair &local, const pubKey &remote) : _local(local), _remote(remote) {}
  template<class Container>
  std::string pack(const Container &mesg) {
    nonce n;

    std::vector<uint8_t> payload;
    payload.resize(capacity(mesg) + MACBytes);

    encrypt(mesg, payload, n, _remote, _local.private_);

    auto payload_hex(util::copy_to<std::string>(to_hex(payload)));
    auto nonce_hex  (util::copy_to<std::string>(to_hex(n)));
    auto public_hex (util::copy_to<std::string>(to_hex(_local.public_)));

    Json::Value pack;

    // No move-semantics :(
    pack["payload"]      = payload_hex;
    pack["nonce"]        = nonce_hex;
    pack["public_key"]   = public_hex;
    pack["payload_size"] = capacity(mesg);

    Json::FastWriter writer;
    return writer.write(pack);
  }

  util::Optional<std::string> unpack(const std::string &mesg);
};
}

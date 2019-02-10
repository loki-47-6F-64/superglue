//
// Created by loki on 29-1-19.
//

#include <kitty/err/err.h>

#include "pack.h"

std::optional<nlohmann::json> pack_candidate(const pj::ice_sess_cand_t &cand) {
  nlohmann::json json;

  switch(cand.type) {
    case pj::ice_cand_type_t::PJ_ICE_CAND_TYPE_HOST:
      json["type"] = "host";
      break;
    case pj::ice_cand_type_t::PJ_ICE_CAND_TYPE_SRFLX:
      json["type"] = "srflx";
      break;
    case pj::ice_cand_type_t::PJ_ICE_CAND_TYPE_RELAYED:
      json["type"] = "relay";
      break;
    default:
      err::set("invalid candidate type");
      return std::nullopt;
  }

  json["foundation"] = pj::string(cand.foundation);
  json["priority"] = cand.prio;

  std::vector<char> buf;
  auto ip_addr = pj::ip_addr_t::from_sockaddr_t(buf, &cand.addr);

  json["ip"] = {
    ip_addr.ip, ip_addr.port
  };

  return json;
}

std::optional<pj::ice_sess_cand_t> unpack_candidate(const nlohmann::json &candidate) {
  auto type = candidate["type"].get<std::string>();

  pj::ice_sess_cand_t cand;
  bzero(&cand, sizeof(cand));

  if(type == "host") {
    cand.type = pj::ice_cand_type_t::PJ_ICE_CAND_TYPE_HOST;
  }
  else if(type == "srflx") {
    cand.type = pj::ice_cand_type_t::PJ_ICE_CAND_TYPE_SRFLX;
  }
  else if(type == "relay") {
    cand.type = pj::ice_cand_type_t::PJ_ICE_CAND_TYPE_RELAYED;
  }
  else {
    err::set("invalid candidate type");

    return std::nullopt;
  }

  auto foundation = candidate["foundation"].get<std::string>();
  cand.foundation = pj::string(foundation);


  cand.prio = candidate["priority"].get<uint32_t>();

  const auto &ip = candidate["ip"];
  auto addr = ip[0].get<std::string>();
  auto port = ip[1].get<std::uint32_t>();
  cand.addr = *pj::ip_addr_t {
    addr,
    port
  }.to_sockaddr();

  cand.addr.addr.sa_family = pj_AF_INET();
  cand.comp_id = 1;

  return cand;
}

std::optional<std::vector<pj::ice_sess_cand_t>> unpack_candidates(const nlohmann::json &candidates) {
  if(candidates.is_null()) {
    err::set("no remote candidates");

    return std::nullopt;
  }

  std::vector<pj::ice_sess_cand_t> result;
  result.reserve(candidates.size());

  for(auto &candidate : candidates) {
    auto unpacked = unpack_candidate(candidate);

    if(!unpacked) {
      return std::nullopt;
    }

    result.emplace_back(*unpacked);
  }

  return result;
}

std::optional<nlohmann::json> pack_remote(const pj::remote_t &remote) {
  nlohmann::json json;

  auto &creds = remote.creds;
  json["ufrag"]  = creds.ufrag;
  json["passwd"] = creds.passwd;

  auto &cand_json = json["candidates"];
  for(const auto &candidate : remote.candidates) {
    auto packed = pack_candidate(candidate);

    if(!packed) {
      return std::nullopt;
    }

    cand_json += *packed;
  }

  return json;
}

std::vector<uuid_t> unpack_peers(const nlohmann::json &peers_json) {
  std::vector<uuid_t> peers;
  peers.reserve(peers_json.size());

  for(auto &peer : peers_json) {
    peers.emplace_back(*util::from_hex<uuid_t>(peer.get<std::string_view>()));
  }

  return peers;
}

std::optional<pj::remote_buf_t> unpack_remote(const nlohmann::json &json) {
  auto candidates = unpack_candidates(json["candidates"]);
  if(!candidates) {
    return std::nullopt;
  }

  return pj::remote_buf_t {
    {
      json["ufrag"].get<std::string>(),
      json["passwd"].get<std::string>()
    },
    std::move(*candidates)
  };
}

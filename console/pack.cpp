//
// Created by loki on 29-1-19.
//

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
  std::vector<pj::ice_sess_cand_t> result;
  result.reserve((std::size_t)std::distance(std::begin(candidates), std::end(candidates)));

  for(auto &candidate : candidates) {
    auto unpacked = unpack_candidate(candidate);

    if(!unpacked) {
      return std::nullopt;
    }

    result.emplace_back(*unpacked);
  }

  return result;
}

std::optional<nlohmann::json> pack(const pj::remote_t &remote) {
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

std::optional<pj::remote_buf_t> unpack_remote(const nlohmann::json &json) {
  auto json_ = nlohmann::json::parse(json.get<std::string>());

  auto candidates = unpack_candidates(json_["candidates"]);
  if(!candidates) {
    return std::nullopt;
  }

  return pj::remote_buf_t {
    {
      json_["ufrag"].get<std::string>(),
      json_["passwd"].get<std::string>()
    },
    std::move(*candidates)
  };
}

std::optional<std::vector<pj::remote_buf_t>> unpack(const nlohmann::json &json) {
  std::vector<pj::remote_buf_t> buf;

  for(auto &json_remote : json) {
    auto remote = unpack_remote(json_remote);

    if(!remote) {
      return std::nullopt;
    }

    buf.emplace_back(std::move(*remote));
  }

  return buf;
}

//
// Created by loki on 8-2-19.
//

#include <map>
#include <kitty/file/io_stream.h>
#include <kitty/util/utility.h>
#include <kitty/log/log.h>
#include <nlohmann/json.hpp>

#include "quest.h"
#include "uuid.h"
#include "pack.h"

using namespace std::string_literals;

void handle_quest(file::io &server);

std::map<uuid_t, file::io> &peers() {
  static std::map<uuid_t, file::io> peers;

  return peers;
}

file::poll_t<file::io, std::monostate> &poll() {
  static file::poll_t<file::io, std::monostate> poll([](file::io &fd, std::monostate) {
    handle_quest(fd);
  }, [](file::io&,std::monostate){}, [](file::io& fd, std::monostate) {
    print(info, "removing connection to the server");

    fd.seal();
  });

  return poll;
}

uuid_t &uuid() {
  static uuid_t uuid { uuid_t::generate() };

  return uuid;
}

pj::ICETrans &ice_trans() {
  static pj::ICETrans ice_trans;

  return ice_trans;
}

void quest_error(file::io &server, const std::string_view &error) {
  nlohmann::json json_error;

  json_error["quest"]   = "error";
  json_error["uuid"]    = util::hex(uuid()).to_string_view();
  json_error["message"] = error;

  auto error_str = json_error.dump();

  print(server, file::raw(util::endian::little((std::uint16_t)error_str.size())), error_str);
}

void send_invite(file::io &server, const std::string_view &uuid) {
  auto err = ice_trans().init_ice(pj::ice_sess_role_t::PJ_ICE_SESS_ROLE_CONTROLLING);
  if(err) {
    print(error, "ice_trans().init_ice(): ", pj::err(err));

    return;
  }

  auto candidates = ice_trans().get_candidates();
  if(candidates.empty()) {
    ice_trans().end_session();
    print(error, "no candidates");

    return;
  }

  auto remote_j = pack_remote({ ice_trans().credentials(), candidates });
  if(!remote_j) {
    print(error, "no remote: ", err::current());

    return;
  }

  nlohmann::json invite;
  invite["quest"]  = "invite";
  invite["uuid"]   = uuid;
  invite["remote"] = *remote_j;

  auto invite_str = invite.dump();
  print(server, file::raw(util::endian::little((std::uint16_t)invite_str.size())), invite_str);
}

void send_register(file::io &server) {
  nlohmann::json register_;

  register_["quest"] = "register";

  auto register_str = register_.dump();

  print(server, file::raw(util::endian::little((std::uint16_t)register_str.size())), register_str);
  auto size = util::endian::little(file::read_struct<std::uint16_t>(server));
  if(!size) {
    print(error, "received no list of peers");
    return;
  }

  auto peers_json = file::read_string(server, *size);
  if(!peers_json) {
    print(error, "received no list of peers");
    return;
  }

  auto peers = unpack_peers(nlohmann::json::parse(*peers_json));

  peers.erase(std::remove(std::begin(peers), std::end(peers), uuid()), std::end(peers));

  print(info, "received a list of ", peers.size(), " peers.");

  if(!peers.empty()) {
    send_invite(server, util::hex(peers[0]).to_string_view());
  }
}

void quest_accept(file::io &server, nlohmann::json &remote_json) {
  auto remote = unpack_remote(remote_json["remote"]);
  if(!remote) {
    ice_trans().end_session();

    print(error, "unpacking remote: ", err::current());
    quest_error(server,  "unpacking remote: "s + err::current());

    return;
  }

  // Attempt connection
  ice_trans().start_ice(*remote);
}

void quest_invite(file::io &server, nlohmann::json &remote_j) {
  auto err = ice_trans().init_ice(pj::ice_sess_role_t::PJ_ICE_SESS_ROLE_CONTROLLED);
  if(err) {
    print(error, "ice_trans().init_ice(): ", pj::err(err));

    return;
  }

  auto candidates = ice_trans().get_candidates();
  if(candidates.empty()) {
    print(error, "no candidates");

    ice_trans().end_session();
    return;
  }

  auto remote_j_send = pack_remote(pj::remote_t {
    ice_trans().credentials(),
    candidates
  });

  if(!remote_j_send) {
    print(error, "no remote: ", err::current());

    ice_trans().end_session();
    return;
  }

  // pack accept
  nlohmann::json accept;
  accept["quest"]  = "accept";
  accept["uuid"]   = remote_j["uuid"];
  accept["remote"] = *remote_j_send;

  auto accept_str = accept.dump();
  print(server, file::raw(util::endian::little((std::uint16_t)accept_str.size())), accept_str);

  auto remote = unpack_remote(remote_j["remote"]);
  if(!remote) {
    ice_trans().end_session();
    print(error, "unpacking remote candidates: ", err::current());

    return;
  }

  ice_trans().start_ice(*remote);
}

void handle_quest(file::io &server) {
  auto size = util::endian::little(file::read_struct<std::uint16_t>(server));

  if(!size) {
    print(error, "Could not read size");

    return;
  }

  try {
    auto remote = nlohmann::json::parse(*read_string(server, *size));

    auto quest = remote["quest"].get<std::string_view>();

    print(debug, "handling quest [", quest, "]");
    if(quest == "error") {
      print(error, "uuid: ", remote["uuid"].get<std::string_view>(), ": message: ", remote["message"].get<std::string_view>());

      return;
    }

    if(quest == "invite") {
      // initiate connection between sender and recipient

      quest_invite(server, remote);
    }

    if(quest == "accept") {
      // tell both peers to start negotiation

      quest_accept(server, remote);
    }

  } catch (const std::exception &e) {
    print(error, "json exception caught: ", e.what());

    return;
  }
}
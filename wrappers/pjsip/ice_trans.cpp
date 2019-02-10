//
// Created by loki on 25-1-19.
//

#include <kitty/err/err.h>
#include <kitty/log/log.h>

#include "ice_trans.h"

namespace pj {

auto constexpr INET6_ADDR_STRING_LEN = PJ_INET6_ADDRSTRLEN;
auto constexpr ICE_MAX_CAND          = PJ_ICE_MAX_CAND;

auto &from_userdata(pj_ice_strans *ice_st) {
  return *reinterpret_cast<ICETrans::func_t::pointer>(pj_ice_strans_get_user_data(ice_st));
}
/*
 * This is the callback that is registered to the ICE stream transport to
 * receive notification about incoming data. By "data" it means application
 * data such as RTP/RTCP, and not packets that belong to ICE signaling (such
 * as STUN connectivity checks or TURN signaling).
 */
void cb_on_rx_data(pj_ice_strans *ice_st,
                          unsigned comp_id,
                          void *data, pj_size_t size,
                          const pj_sockaddr_t *src_addr,
                          unsigned src_addr_len) {
  auto &func = std::get<0>(from_userdata(ice_st));


  std::vector<char> buf;
  func({
    ice_st, {
      ip_addr_t::from_sockaddr_t(buf, src_addr)
    }},
    std::string_view { (char*)data, size });
}

/*
 * This is the callback that is registered to the ICE stream transport to
 * receive notification about ICE state progression.
 */
void cb_on_ice_complete(pj_ice_strans *ice_st,
                               ice_trans_op_t op,
                               status_t status) {
  auto &funcs = from_userdata(ice_st);

  if(op == pj::ice_trans_op_t::PJ_ICE_STRANS_OP_NEGOTIATION) {
    ice_sess_cand_t cand;

    pj_ice_strans_get_def_cand(ice_st, 1, &cand);

    std::vector<char> buf;
    std::get<2>(funcs)({
      ice_st, {
        ip_addr_t::from_sockaddr_t(buf, &cand.addr)
      }
    }, status);

    return;
  }

  if(op == pj::ice_trans_op_t::PJ_ICE_STRANS_OP_INIT) {
    ice_sess_cand_t cand;

    pj_ice_strans_get_def_cand(ice_st, 1, &cand);

    std::vector<char> buf;
    std::get<1>(funcs)({
      ice_st, {
        ip_addr_t::from_sockaddr_t(buf, &cand.addr)
      }
    }, status);

    return;
  }

  auto *state_name = op == PJ_ICE_STRANS_OP_KEEP_ALIVE ? "ICE_STRANS_OP_KEEP_ALIVE" : "ICE_STRANS_OP_ADDR_CHANGE";

  print(warning, "unhandled state change [", state_name, ":", status == success ? "success" : "fail");
}

constexpr ice_trans_cb_t ice_trans_cb {
  cb_on_rx_data,
  cb_on_ice_complete
};

ICETrans::ICETrans(const ice_trans_cfg_t &ice_trans_cfg, func_t &&callbacks) : _ice_cb { std::move(callbacks) } {

  pj_ice_strans *ptr;

  pj_ice_strans_create(nullptr, &ice_trans_cfg, 1, _ice_cb.get(), &ice_trans_cb, &ptr);

  _ice_trans.reset(ptr);
}

status_t ICETrans::init_ice(pj::ice_sess_role_t role) {
  if(get_state().has_session()) {
    return success;
  }

  return pj_ice_strans_init_ice(_ice_trans.get(), role, nullptr, nullptr);
}

creds_t ICETrans::credentials() {
  str_t ufrag;
  str_t passwd;

  pj_ice_strans_get_ufrag_pwd(_ice_trans.get(), &ufrag, &passwd, nullptr, nullptr);

  return creds_t { string(ufrag), string(passwd) };
}

std::vector<ice_sess_cand_t> ICETrans::get_candidates(unsigned int comp_cnt) {
  ice_sess_cand_t cand[ICE_MAX_CAND];
  unsigned count = ICE_MAX_CAND;

  pj_ice_strans_enum_cands(_ice_trans.get(), comp_cnt +1, &count, cand);

  return { cand, cand + count };
}

status_t _start_ice(ice_trans_t::pointer const ice_trans, const creds_t &creds, const std::vector<ice_sess_cand_t> &candidates) {
  auto ufrag  = string(creds.ufrag);
  auto passwd = string(creds.passwd);

  return pj_ice_strans_start_ice(ice_trans, &ufrag, &passwd, (unsigned)candidates.size(), candidates.data());
}

status_t ICETrans::start_ice(const remote_t &remote) {
  return _start_ice(_ice_trans.get(), remote.creds, remote.candidates);
}

status_t ICETrans::start_ice(const remote_buf_t &remote) {
  return _start_ice(_ice_trans.get(), remote.creds, remote.candidates);
}

status_t ICETrans::set_role(ice_sess_role_t role) {
  return pj_ice_strans_change_role(_ice_trans.get(), role);
}

status_t ICETrans::end_call() {
  if(auto err_code = pj_ice_strans_stop_ice(_ice_trans.get())) {
    return err_code;
  }

  return init_ice();
}

status_t ICETrans::end_session() {
  return pj_ice_strans_destroy(_ice_trans.get());
}

ICEState ICETrans::get_state() const {
  return ICEState {
    pj_ice_strans_get_state(_ice_trans.get())
  };
}

ip_addr_t ip_addr_t::from_sockaddr_t(std::vector<char> &buf, const sockaddr_t *const ip_addr) {
  buf.resize(INET6_ADDR_STRING_LEN);
  pj_sockaddr_print(ip_addr, buf.data(), (int)buf.size(), 0);\

  return ip_addr_t { buf.data(), pj_sockaddr_get_port(ip_addr) };
}

std::optional<sockaddr> ip_addr_t::to_sockaddr() {
  sockaddr result;

  auto addr = string(ip);
  if(auto err_code = pj_sockaddr_init(pj_AF_INET(), &result, &addr, (std::uint16_t)port)) {
    err::set(err(err_code));

    return std::nullopt;
  }

  return result;
}

ICECall::ICECall(ice_trans_t::pointer ice_trans, ip_addr_t ip_addr) : ip_addr(ip_addr), _ice_trans(ice_trans) {}

status_t ICECall::end_call() {
  if(auto err_code = pj_ice_strans_stop_ice(_ice_trans)) {
    return err_code;
  }

  return pj_ice_strans_init_ice(_ice_trans, ice_sess_role_t::PJ_ICE_SESS_ROLE_CONTROLLED, nullptr, nullptr);
}

status_t ICECall::set_role(ice_sess_role_t role) {
  return pj_ice_strans_change_role(_ice_trans, role);
}

status_t ICECall::start_ice(const remote_t &remote) {
  return _start_ice(_ice_trans, remote.creds, remote.candidates);
}

status_t ICECall::start_ice(const remote_buf_t &remote) {
  return _start_ice(_ice_trans, remote.creds, remote.candidates);
}

creds_t ICECall::credentials() {
  str_t ufrag;
  str_t passwd;

  pj_ice_strans_get_ufrag_pwd(_ice_trans, &ufrag, &passwd, nullptr, nullptr);

  return creds_t { string(ufrag), string(passwd) };
}

std::vector<ice_sess_cand_t> ICECall::get_candidates(unsigned int comp_cnt) {
  ice_sess_cand_t cand[ICE_MAX_CAND];
  unsigned count;

  pj_ice_strans_enum_cands(_ice_trans, comp_cnt +1, &count, cand);

  return { cand, cand + count };
}

ICEState ICECall::get_state() const {
  return ICEState {
    pj_ice_strans_get_state(_ice_trans)
  };
}

creds_buf_t::operator creds_t() const {
  return creds_t {
    ufrag,
    passwd
  };
}

bool ICEState::created() const {
  return !(_state == ice_trans_state_t::PJ_ICE_STRANS_STATE_NULL);
}

bool ICEState::initializing() const {
  return _state == ice_trans_state_t::PJ_ICE_STRANS_STATE_INIT;
}

bool ICEState::initialized() const {
  return _state == ice_trans_state_t::PJ_ICE_STRANS_STATE_READY;
}

bool ICEState::has_session() const {
  return _state == ice_trans_state_t::PJ_ICE_STRANS_STATE_SESS_READY;
}

bool ICEState::connecting() const {
  return _state == ice_trans_state_t::PJ_ICE_STRANS_STATE_NEGO;
}

bool ICEState::connected() const {
  return _state == ice_trans_state_t::PJ_ICE_STRANS_STATE_RUNNING;
}

bool ICEState::failed() const {
  return _state == ice_trans_state_t::PJ_ICE_STRANS_STATE_FAILED;
}

ICEState::ICEState(pj_ice_strans_state state) : _state(state) {}
}

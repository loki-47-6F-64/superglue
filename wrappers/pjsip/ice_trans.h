//
// Created by loki on 25-1-19.
//

#ifndef T_MAN_ICESTRANS_H
#define T_MAN_ICESTRANS_H

#include <memory>
#include <optional>
#include <functional>

#include "nath.h"
#include <pjnath.h>

namespace pj {

using sockaddr_t = pj_sockaddr_t;
using sockaddr   = pj_sockaddr;
using ice_trans_cfg_t = pj_ice_strans_cfg;
using ice_trans_cb_t  = pj_ice_strans_cb;

using ice_sess_role_t   = pj_ice_sess_role;
using ice_trans_op_t    = pj_ice_strans_op;
using ice_trans_state_t = pj_ice_strans_state;

struct ip_addr_t {
  std::string_view ip;
  std::uint32_t port;

  std::optional<sockaddr> to_sockaddr();
  static ip_addr_t from_sockaddr_t(std::vector<char> &buf, const sockaddr_t* ip_addr);
};

struct creds_t {
  std::string_view ufrag;
  std::string_view passwd;
};

struct creds_buf_t {
  std::string ufrag;
  std::string passwd;

  creds_buf_t(creds_buf_t&&) noexcept = default;
  creds_buf_t&operator=(creds_buf_t&&) noexcept = default;

  operator creds_t() const;
};

struct remote_t {
  creds_t creds;
  std::vector<ice_sess_cand_t> candidates;
};

struct remote_buf_t {
  creds_buf_t creds;
  std::vector<ice_sess_cand_t> candidates;
};

class ICEState {
public:
  explicit ICEState(pj_ice_strans_state);

  /**
   * ICE stream transport is not created.
   */
  bool created() const;

  /**
   * Gathering ICE candidates
   */
  bool initializing() const;

  /**
   * ICE stream transport initialization/candidate gathering process is
   * complete, ICE session may be created on this stream transport.
   */
  bool initialized() const;

  /**
   * New session has been created and the session is ready.
   */
  bool has_session() const;

  /**
   * Negotiating
   */
  bool connecting() const;

  /**
   * ICE negotiation has completed successfully.
   */
  bool connected() const;

  /**
   * ICE negotiation has completed with failure.
   */
  bool failed() const;
private:
  ice_trans_state_t _state;
};

class ICECall {
public:
  ICECall(ice_trans_t::pointer ice_trans, ip_addr_t ip_addr);

  status_t set_role(ice_sess_role_t role);
  status_t end_call();

  status_t start_ice(const remote_t &remote);
  status_t start_ice(const remote_buf_t &remote);

  creds_t credentials();

  ICEState get_state() const;
  std::vector<ice_sess_cand_t> get_candidates(unsigned comp_cnt = 0);

  template<class T>
  status_t send(util::FakeContainer<T> data) {
    auto sock { *ip_addr.to_sockaddr() };

    return pj_ice_strans_sendto(_ice_trans, 1, data.data(), (std::size_t)std::distance(std::begin(data), std::end(data)), &sock, sizeof(sock));
  }

  ip_addr_t ip_addr;
private:
  ice_trans_t::pointer _ice_trans;
};

class ICETrans {
public:
  using func_t = std::unique_ptr<std::tuple<
    std::function<void(ICECall, std::string_view)>,
    std::function<void(ICECall, status_t)>,
    std::function<void(ICECall, status_t)>>
  >;

  ICETrans() = default;

  explicit ICETrans(const ice_trans_cfg_t &ice_trans_cfg, func_t &&callback);

  status_t init_ice(ice_sess_role_t role = ice_sess_role_t::PJ_ICE_SESS_ROLE_CONTROLLED);
  status_t set_role(ice_sess_role_t role);

  status_t end_call();
  status_t end_session();

  status_t start_ice(const remote_t &remote);
  status_t start_ice(const remote_buf_t &remote);

  ICEState get_state() const;

  creds_t credentials();

  std::vector<ice_sess_cand_t> get_candidates(unsigned comp_cnt = 0);
private:
  func_t _ice_cb;

  ice_trans_t _ice_trans;

  /*
 * This is the callback that is registered to the ICE stream transport to
 * receive notification about incoming data. By "data" it means application
 * data such as RTP/RTCP, and not packets that belong to ICE signaling (such
 * as STUN connectivity checks or TURN signaling).
 */
  friend void cb_on_rx_data(pj_ice_strans *,
                     unsigned comp_id,
                     void *data, pj_size_t,
                     const pj_sockaddr_t *,
                     unsigned);

  /*
 * This is the callback that is registered to the ICE stream transport to
 * receive notification about ICE state progression.
 */
  friend void cb_on_ice_complete(pj_ice_strans *,
                          pj_ice_strans_op,
                          pj_status_t);
};

}

#endif //T_MAN_ICESTRANS_H

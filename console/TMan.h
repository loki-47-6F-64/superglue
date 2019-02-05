//
// Created by loki on 31-12-18.
//

#ifndef T_MAN_TMAN_H
#define T_MAN_TMAN_H


#include <vector>
#include <functional>
#include <random>
#include <algorithm>

template<class T>
std::vector<T> _merge(const std::vector<T> &left, const T &right) {
  std::vector<T> merged;
  merged.reserve(left.size() +1);

  merged.insert(std::end(merged), std::begin(left), std::end(left));
  merged.emplace_back(right);

  return merged;
}

template<class T>
std::vector<T> _merge(const std::vector<T> &left, const std::vector<T> &right) {
  std::vector<T> merged;
  merged.reserve(left.size() + right.size());

  merged.insert(std::end(merged), std::begin(left), std::end(left));
  merged.insert(std::end(merged), std::begin(right), std::end(right));

  return merged;
}

template<class T, class X>
std::vector<T> _merge(const std::vector<T> &left, const std::vector<X> &right) {
  std::vector<T> merged;
  merged.reserve(left.size() + right.size());

  merged.insert(std::end(merged), std::begin(left), std::end(left));
  merged.insert(std::end(merged), std::begin(right), std::end(right));

  return merged;
}

template<std::size_t D, class T>
static void rand_seq(std::array<T, D> &seq, std::default_random_engine &engine) {
  for(std::size_t x = 0; x < seq.size(); ++x) {
    seq[x] = x;
  }

  std::shuffle(std::begin(seq), std::end(seq), engine);
}


template<class T>
class TMan {
public:
  using rank_t = T;

  struct descriptor_t {
    rank_t rank;

    TMan *address;
    TMan *operator->() {
      return address;
    }
  };

  struct rand_descriptor_t : public descriptor_t {
    static rand_descriptor_t from_descriptor(const descriptor_t &descr) {
      return rand_descriptor_t {
        descr.rank,
        descr.address,
        0
      };
    }

    std::uint32_t hop_count;
  };

  TMan() = default;
  TMan(TMan && other) noexcept {
    *this = std::move(other);

    _descr.address = this;
  };


  TMan &operator=(TMan && other) noexcept {
    std::swap(_C, other._C);
    std::swap(_peers, other._peers);
    std::swap(_descr.rank, other._descr.rank);

    // distance function is the same
    // it makes no sense to swap the random engine

    return *this;
  };

  TMan(const rank_t &rank, std::default_random_engine &_rand_engine,
        const std::function<int(const rank_t &, const rank_t &)> &distance, size_t _C) : _rand_engine(&_rand_engine),
                                                                                         distance(distance), _C(_C) {
    _descr = { rank, this };
  }

  /**
   * Simulate passage of time
   */
  void iterate() {
    auto peer = _select_peer();

    auto buf_p = peer->_recv(_merge(_merge(_peers, _descr), _rand_peers));

    auto buf = _merge(_peers, buf_p);

    _erase_self(buf);
    _peers = _select_view(std::move(buf));
  }

  /**
   * Simulate passage of time for peer sampling
   */
  void rand_iterate() {
    auto peer = _rand_select_peer();

    auto view_q = peer->_rand_recv(_merge(_rand_peers, rand_descriptor_t::from_descriptor(_descr)));

    _erase_self(view_q);

    _inc_hopcount(view_q);

    _rand_peers = _rand_select_view(_merge(_rand_peers, view_q));
  }

private:
  void _sort_peers(std::vector<descriptor_t> &peers) {
    std::sort(std::begin(peers), std::end(peers), [this](auto &l, auto &r) {
      return std::abs(distance(_descr.rank, l.rank)) < std::abs(distance(_descr.rank, r.rank));
    });
  }

  /**
   * remove any occurences of this in the list of peers
   * @param buf
   */
  template<class X>
  void _erase_self(std::vector<X> &buf) {
    buf.erase(std::remove_if(std::begin(buf), std::end(buf), [this](auto &descr) { return descr.address == _descr.address; }), std::end(buf));
  }

  /**
   * Simulate receiving and accepting communication from peer
   * @param buf_q A list of potential peers
   * @return The list of peers
   */
  std::vector<descriptor_t> _recv(std::vector<descriptor_t> &&buf_q) {
    _erase_self(buf_q);

    auto buf = _merge(_merge(_peers, _descr), _rand_peers);

    _peers = _select_view(_merge(_peers, buf_q));
    return buf;
  }

  /**
   * Simulate receiving and accepting communication for peer sampling
   * @param view_p A list of random peers
   * @return Our list of random peers
   */
  std::vector<rand_descriptor_t> _rand_recv(std::vector<rand_descriptor_t> &&view_p) {
    _inc_hopcount(view_p);

    auto descr = rand_descriptor_t::from_descriptor(_descr);

    auto view_q = _merge(_rand_peers, descr);

    _rand_peers = _rand_select_view(_merge(_rand_peers, view_p));

    return view_q;
  }

  /**
   *
   * @param view All peers to be selected from
   * @return a list of peers sorted from close to far
   */
  std::vector<descriptor_t> _select_view(std::vector<descriptor_t> &&view) {
    _sort_peers(view);

    std::array<std::vector<descriptor_t>, 2> directed;
    for(const auto &desc : view) {
      if(distance(desc.rank, _descr.rank) > 0) {
        directed[0].emplace_back(desc);
      }
      else {
        directed[1].emplace_back(desc);
      }
    }

    // sort the directed nodes
    std::vector<descriptor_t> new_view(_C);

    auto min_size = std::min(directed[0].size(), directed[1].size());

    for(std::size_t x = 0; x < min_size && x*2 < _C -1; ++x) {
      std::array<std::size_t, 2> seq({0, 0});
      rand_seq(seq, *_rand_engine);

      new_view[2*x + seq[0]] = directed[0][x];
      new_view[2*x + seq[1]] = directed[1][x];
    }

    for(auto x = 0; x < directed[0].size() && 2*min_size + x < _C; ++x) {
      new_view[2*min_size + x] = directed[0][x];
    }

    for(auto x = 0; x < directed[1].size() && 2*min_size + x < _C; ++x) {
      new_view[2*min_size + x] = directed[1][x];
    }

    return new_view;
  }

  /**
   * The method of selecting the view has a serious inpact on the distribution of the peer samples across the network
   * @param view All peers to be selected from
   * @return a list of peers sorted from small hops to big hops
   */
  std::vector<rand_descriptor_t> _rand_select_view(std::vector<rand_descriptor_t> &&view) {
    std::sort(std::begin(view), std::end(view), [](auto &l, auto &r) { return l.hop_count < r.hop_count; });

    if(view.size() > _C) {
      view.resize(_C);
    }

    return std::move(view);
  }

  const rand_descriptor_t &_rand_select_peer() const {
    return _rand_peers.back();
  }

  const descriptor_t &_select_peer() const {
    return _peers.front();
  }

  void _inc_hopcount(std::vector<rand_descriptor_t> &view) {
    for(auto &descr : view) {
      ++descr.hop_count;
    }
  }

  descriptor_t _descr;

  std::vector<descriptor_t> _peers;
  std::vector<rand_descriptor_t> _rand_peers;

  std::default_random_engine *_rand_engine;

  std::function<int(const rank_t &, const rank_t &)> distance;

  std::size_t _C;

public:
  void set_peers(std::vector<descriptor_t> &&peers) {
    _peers = std::move(peers);

    _sort_peers(_peers);
  }

  void set_rand_peers(std::vector<descriptor_t> &&peers) {
    _rand_peers.reserve(peers.size());

    for(auto &peer : peers) {
      _rand_peers.emplace_back(rand_descriptor_t { peer.rank, peer.address, 1 });
    }
  }

  const std::vector<descriptor_t> &get_peers() const {
    return _peers;
  }

  const descriptor_t &get_descr() const {
    return _descr;
  }
};

#endif //T_MAN_TMAN_H

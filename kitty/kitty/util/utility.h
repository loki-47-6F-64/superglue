#ifndef UTILITY_H
#define UTILITY_H

#include <vector>
#include <memory>

#include <kitty/util/optional.h>
#include <kitty/file/file.h>

namespace util {

template<class T>
void append_struct(std::vector<uint8_t> &buf, T &_struct) {
  constexpr size_t data_len = sizeof(_struct);

  uint8_t *data = (uint8_t *) & _struct;

  for (size_t x = 0; x < data_len; ++x) {
    buf.push_back(data[x]);
  }
}

template<class T, class Stream>
Optional<T> read_struct(file::FD<Stream> &io) {
  constexpr size_t data_len = sizeof(T);
  uint8_t buf[data_len];

  size_t x = 0;
  int err = io.eachByte([&](uint8_t ch) {
    buf[x++] = ch;

    return x < data_len ? err::OK : err::BREAK;
  });

  T *val = (T*)buf;
  return err ? Optional<T>() : Optional<T>(*val);
}
  
template<class T>
class Hex {
public:
  typedef T elem_type;
private:
  const char _bits[16] {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
  };

  uint8_t _hex[sizeof(elem_type) * 2];
public:
  Hex(elem_type && elem) {
    const uint8_t *data = reinterpret_cast<const uint8_t *>(&elem) + sizeof(elem_type);
    for (elem_type *it = begin(); it < cend();) {
      *it++ = _bits[*--data / 16];
      *it++ = _bits[*data   % 16];
    }
  }

  Hex(elem_type &elem) {
    const uint8_t *data = reinterpret_cast<const uint8_t *>(&elem) + sizeof(elem_type);
    for (uint8_t *it = begin(); it < cend();) {
      *it++ = _bits[*--data / 16];
      *it++ = _bits[*data % 16];
    }
  }

  uint8_t *begin() { return _hex; }
  uint8_t *end() { return _hex + sizeof(elem_type) * 2; }

  const uint8_t *cbegin() { return _hex; }
  const uint8_t *cend() { return _hex + sizeof(elem_type) * 2; }
};

template<class T>
Hex<T> hex(T &elem) {
  return Hex<T>(elem);
}

template<class T, class... Args>
std::unique_ptr<T> mk_uniq(Args && ... args) {
  typedef T elem_type;

  return std::unique_ptr<elem_type> {
    new elem_type { std::forward<Args>(args)... }
  };
}

template<class T>
using Error = Optional<T>;

template<class ReturnType, class ...Args>
struct Function {
  typedef ReturnType (*type)(Args...);
};

template<class T, class ReturnType, typename Function<ReturnType, T>::type function>
struct Destroy {
  typedef T pointer;
  
  void operator()(pointer p) {
    function(p);
  }
};

template<class T, typename Function<void, T*>::type function>
using safe_ptr = std::unique_ptr<T, Destroy<T*, void, function>>;

// You cannot specialize an alias
template<class T, class ReturnType, typename Function<ReturnType, T*>::type function>
using safe_ptr_v2 = std::unique_ptr<T, Destroy<T*, ReturnType, function>>;


template<class T>
class FakeContainer {
  typedef T* pointer;

  pointer _begin;
  pointer _end;

public:
  FakeContainer(pointer begin, pointer end) : _begin(begin), _end(end) {}

  pointer begin() { return _begin; }
  pointer end() { return  _end; }

  const pointer begin() const { return _begin; }
  const pointer end() const { return _end; }

  const pointer cbegin() const { return _begin; }
  const pointer cend() const { return _end; }

  pointer data() { return begin(); }
  const pointer data() const { return cbegin(); }
};

template<class T>
FakeContainer<T> toContainer(T * const begin, T * const end) {
  return { begin, end };
}

template<class T>
FakeContainer<T> toContainer(T * const begin) {
  T *end = begin;

  auto default_val = T();
  while(*end != default_val) {
    ++end;
  }

  return toContainer(begin, end);
}

}
#endif

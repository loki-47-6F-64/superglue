#ifndef KITTY_UTIL_STRING_H
#define KITTY_UTIL_STRING_H

// Android NDK support
#include <sstream>
namespace std {
  template<class T>
  std::string to_string(const T& val) {
    std::stringstream ss;

    ss << val;

    return ss.str();
  }
}

#endif

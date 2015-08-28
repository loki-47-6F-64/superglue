#ifndef DOSSIER_PROXY_H
#define DOSSIER_PROXY_H

#include <vector>
#include <string>

#include <memory>

#include <kitty/err/err.h>
#include <kitty/file/file.h>
#include <kitty/server/server.h>
#include <kitty/util/utility.h>

namespace server {
namespace proxy {
/*
 * All values in the request are send in a null-terminated byte string
 * On failure a non-zero value is returned and err_msg is set.
 */

template<class File>
int load(File &socket) {
  if (socket.eof()) {
    err::code = err::FILE_CLOSED;
    return -1;
  }

  return 0;
}

template<class File, class... Args>
int load(File &socket, std::string &buf, int max, Args && ... params) {
  int err = socket.eachByte([&](unsigned char ch) {
    if (buf.size() > max) {
      return err::OUT_OF_BOUNDS;
    }

    if (!ch) {
      return err::BREAK;
    }

    buf.push_back(ch);
    return err::OK;
  });

  if (err) {
    return -1;
  }

  return load(socket, std::forward<Args>(params)...);
}

template<class File, class... Args>
int load(File &socket, int64_t &buf, Args && ... params) {
  constexpr int max_digits = 10;

  std::string str;
  if (load(socket, str, max_digits)) {
    return -1;
  }

  buf = std::atol(str.c_str());
  return load(socket, std::forward<Args>(params)...);
}

template<class File, class... Args>
int load(File &socket, int &buf, Args && ... params) {
  constexpr int max_digits = 10;

  std::string str;
  if (load(socket, str, max_digits)) {
    return -1;
  }

  buf = std::atoi(str.c_str());
  return load(socket, std::forward<Args>(params)...);
}

template<class File, class... Args>
int load(
  File &socket,
  std::vector<std::string> &vs, int max_params, const int max_size,
  Args && ... params) {

  for (int x = 0; x < max_params; ++x) {
    std::string tmp;
    if (load(socket, tmp, max_size)) {
      return -1;
    }

    if (tmp.empty()) {
      break;
    }

    vs.push_back(std::move(tmp));
  }

  return load(socket, std::forward<Args>(params)...);
}

template<class File, class... Args>
int load(File &socket,
         std::vector<std::pair<int, std::string>> &vis,
         const int max_params, const int max_size, Args && ... params) {

  for (int x = 0; x < max_params; ++x) {
    std::string tmp;
    if (load(socket, tmp, max_size)) {
      return -1;
    }

    if (tmp.empty()) {
      break;
    }

    int room_number;
    if (load(socket, room_number)) {
      return -1;
    }

    vis.emplace_back(room_number, std::move(tmp));
  }

  return load(socket, std::forward<Args>(params)...);
}
}
}
#endif

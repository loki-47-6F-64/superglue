//
// Created by loki on 24-1-19.
//

#include <string>
#include <array>

#include "nath.h"
#include "pool.h"

#include <kitty/util/optional.h>
#include <kitty/log/log.h>

namespace pj {

// The application pool
Pool pool;


// log callback to write to file
static void log_func(int level, const char *data, int len) {
  std::string_view str { data, (std::size_t)len };

  if(str.back() == '\n') {
    str.remove_suffix(1);
  }

  switch (level) {
    case 0:
      print(error, "FATAL --> aborting... ", str);
      std::abort();
    case 1:
      print(info, str);
      break;
    case 2:
      print(info, str);
      break;
    case 3:
      print(info, str);
      break;
    case 4:
      print(info, str);
      break;
    case 5:
      print(debug, str);
      break;
  }
}

thread_t register_thread() {
  auto desc = std::make_unique<std::array<long, THREAD_DESC_SIZE>>();

  pj_thread_t *thread_raw;
  pj_thread_register(nullptr, desc->data(), &thread_raw);
  return thread_t {
    std::move(desc),
    thread_ptr { thread_raw }
  };
}

status_t init(util::Optional<std::string_view> logFile) {
  pj_log_set_level(5);
  if(logFile) {
    pj_log_set_log_func(&pj_log_write);
  }

  pj_log_set_log_func(&log_func);

  if(auto status = pj_init()) {
    return status;
  }

  if(auto status = pjlib_util_init()) {
    return status;
  }

  if(auto status = pjnath_init()) {
    return status;
  }
  return success;
}

str_t string(const std::string_view &str) {
  return str_t { const_cast<char*>(str.data()), (ssize_t)str.size() };
}

std::string_view string(const str_t &str) {
  return std::string_view { str.ptr, (std::size_t)str.slen };
}

void dns_resolv_destroy(pj_dns_resolver *ptr) {
  pj_dns_resolver_destroy(ptr, False);
}

std::chrono::milliseconds time(const time_val_t& duration) {
  return std::chrono::milliseconds(duration.msec) + std::chrono::seconds(duration.sec);
}

std::string err(status_t err_code) {
  auto err_str = std::make_unique<char[]>(PJ_ERR_MSG_SIZE);

  pj_strerror(err_code, err_str.get(), PJ_ERR_MSG_SIZE);

  return { err_str.release(), PJ_ERR_MSG_SIZE };
}

func_err_t get_netos_err = pj_get_netos_error;
func_err_t get_os_err = pj_get_os_error;

}

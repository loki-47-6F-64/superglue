#include <errno.h>
#include <string.h>
#include <array>

#ifdef KITTY_BUILD_SSL
#include <openssl/err.h>
#endif

#include <kitty/err/err.h>

namespace err {

constexpr int MAX_ERROR_BUFFER = 120;
THREAD_LOCAL util::ThreadLocal<char[MAX_ERROR_BUFFER]> err_buf { '\0' };
THREAD_LOCAL util::ThreadLocal<code_t> code { OK };

// Support int strerr_r and char *strerr_r
template<class T, class S = void>
struct _getSysError;

template<class T>
struct _getSysError<T, typename std::enable_if<std::is_pointer<typename std::decay<T>::type>::value>::type> {
  /* Using 'int' directly causes syntax error on clang in xcode
   * It tries to convert 'const char*' to 'int'.
   * This function will be called only if strerror_r returns an integer
   */
  static typename std::decay<T>::type value() {
    return strerror_r(errno, err_buf, MAX_ERROR_BUFFER);
  }
};

template<class T>
struct _getSysError<T, typename std::enable_if<std::is_integral<typename std::remove_reference<T>::type>::value>::type> {
  static const char *value() {
    strerror_r(errno, err_buf, MAX_ERROR_BUFFER);

    return err_buf;
  }
};

typedef _getSysError<decltype(strerror_r(errno, err_buf, MAX_ERROR_BUFFER))> getSysError;
const char *sys() {
  
  return getSysError::value();
}

const char *ssl() {
#ifdef KITTY_BUILD_SSL
  auto err = ERR_get_error();
  if(err) {
    ERR_error_string_n(err, err_buf, MAX_ERROR_BUFFER);
    return err_buf;
  }

  return "";
#else
  return "SSL support is not compiled with the library";
#endif
}

void _set(const char *src) {
  int x;
  for(x = 0; src[x] && x < MAX_ERROR_BUFFER - 1; ++x) {
    err_buf[x] = src[x];
  }
  err_buf[x] = '\0';
}

void set(const char *error) {
  err::code = LIB_USER;
  
  return _set(error);
}

void set(std::string &error) {
  return set(error.c_str());
}
void set(std::string &&error) {
  return set(error);
}

const char *current() {
  switch(code) {
    case OK:
      return "Success";
    case TIMEOUT:
      return "Timeout occurred";
    case FILE_CLOSED:
      return "Connection prematurely closed";
    case INPUT_OUTPUT:
      return "Input/Output error";
    case OUT_OF_BOUNDS:
      return "Out of bounds";
    case BREAK:
      return "BREAK";
    case UNAUTHORIZED:
      return "unauthorized";
    case LIB_USER:
      // Special exception: error_code is returned to the caller,
      // the caller must set the error message
      return err_buf;
    case LIB_SSL:
      return ssl();
    case LIB_SYS:
      return sys();
  };
  
  return "Unknown Error";
}
}

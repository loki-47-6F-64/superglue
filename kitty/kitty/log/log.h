#ifndef DOSSIER_LOG_H
#define DOSSIER_LOG_H

#include <ctime>
#include <vector>
#include <string>
#include <mutex>

#include <kitty/file/io_stream.h>
#include <kitty/util/thread_local.h>

#ifdef KITTY_DEBUG
#define ON_DEBUG( x ) x                                                                                                                 
#define DEBUG_LOG( ... ) print(debug, __FILE__, ':', __LINE__,':', __VA_ARGS__)                                                         
#else                                                                                                                                     
#define ON_DEBUG( ... )                                                                                                                 
#define DEBUG_LOG( ... ) do {} while(0)                                                                                                   
#endif

namespace file {
namespace stream {
constexpr int DATE_BUFFER_SIZE = 21 + 1; // Full string plus '\0'

extern THREAD_LOCAL util::ThreadLocal<char[DATE_BUFFER_SIZE]> _date;

template<class Stream>
class Log {
  Stream _stream;

  std::string _prepend;
public:

  Log() = default;
  Log(Log &&other) = default;

  Log &operator =(Log&& stream) = default;

  template<class... Args>
  Log(std::string&& prepend, Args&&... params) : _stream(std::forward<Args>(params)...), _prepend(std::move(prepend)) {}

  int read(std::vector<unsigned char>& buf) {
    return -1;
  }

  int write(std::vector<unsigned char>& buf) {
    std::time_t t = std::time(NULL);
    strftime(_date, DATE_BUFFER_SIZE, "[%Y:%m:%d:%H:%M:%S]", std::localtime(&t));

    buf.insert(buf.begin(), _prepend.cbegin(), _prepend.cend());

    buf.insert(
      buf.begin(),
      _date.get(),
      _date.get() + DATE_BUFFER_SIZE - 1 //omit '\0'
    );

    buf.push_back('\n');
    return _stream.write(buf);
  }

  bool is_open() {
    return _stream.is_open();
  }

  bool eof() {
    return false;
  }

  int access(const char *path, std::function<int(Stream &, const char *)> open) {
    return open(_stream, path);
  }

  void seal() {
    _stream.seal();
  }

  int fd() {
    return _stream.fd();
  }
};

}
extern void log_open(const char *logPath);

typedef FD<stream::Log<stream::io>> Log;
}

extern file::Log error;
extern file::Log warning;
extern file::Log info;
extern file::Log debug;
#endif

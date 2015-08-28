#ifndef IO_STREAM_H
#define IO_STREAM_H

#include <string>
#include <kitty/file/file.h>
namespace file {
namespace stream {
class io {
  bool _eof;
  int _fd;

public:
  io();
  io(int fd);

  void operator =(io&& stream);

  int read(std::vector<unsigned char>& buf);
  int write(std::vector<unsigned char>& buf);

  bool is_open();
  bool eof();

  void seal();

  int fd();
};
}
typedef FD<stream::io> io;

io ioRead(const char *file_path);
io ioRead(std::string &file_path);
io ioRead(std::string &&file_path);

io ioWrite(const char *file_path);
io ioWrite(std::string &file_path);
io ioWrite(std::string &&file_path);

io ioWriteAppend(const char *file_path);
io ioWriteAppend(std::string &file_path);
io ioWriteAppend(std::string &&file_path);
}

#endif
#ifndef SSL_STREAM_H
#define SSL_STREAM_H

#include <openssl/ssl.h>
#include <kitty/file/file.h>
#include <kitty/util/utility.h>

namespace file {
namespace stream {

typedef util::safe_ptr<SSL, SSL_free> _SSL;
typedef util::safe_ptr<SSL_CTX, SSL_CTX_free> Context;

class ssl {
  bool _eof;

public:
  _SSL _ssl;

  ssl();
  ssl(Context &ctx, int fd);

  void operator=(ssl&& stream);

  int read(std::vector<unsigned char>& buf);
  int write(std::vector<unsigned char>& buf);

  bool is_open();
  bool eof();

  void seal();

  int fd();
};
}


typedef FD<stream::ssl> ssl;
}

#endif
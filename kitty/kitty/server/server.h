#ifndef DOSSIER_SERVER_H
#define DOSSIER_SERVER_H

#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <vector>
#include <mutex>
#include <tuple>

#include <kitty/util/thread_pool.h>
#include <kitty/util/optional.h>
#include <kitty/util/move_by_copy.h>
#include <kitty/util/auto_run.h>

#include <kitty/file/file.h>

#include <kitty/log/log.h>
#include <kitty/err/err.h>
namespace server {


template<typename>
struct DefaultType {
  typedef char Type[0];
};

template<class T>
class Server {
public:
  typedef T Client;
  typedef typename Client::_sockaddr _sockaddr;
  typedef typename DefaultType<Client>::Type Member;

private:
  pollfd _listenfd;

  util::ThreadPool _task;
  
  util::AutoRun<void> _autoRun;

  Member _member;
public:

  Server() : _task(1) {
    static_assert(sizeof(Member) == 0, "Default constructor cannot be used when DefaultType is overriden");
  }

  Server(Member&& member) : _task(1), _member(std::move(member)) { }

  ~Server() { stop(); }
  
  // Returns -1 on failure
  int start(_sockaddr &server, std::function<void(Client &&)> f) {
    pollfd pfd {
      _socket(),
      POLLIN,
      0
    };

    if(pfd.fd == -1) {
      err::code = err::LIB_SYS;
      return -1;
    }

    // Allow reuse of local addresses
    if(setsockopt(pfd.fd, SOL_SOCKET, SO_REUSEADDR, &pfd.fd, sizeof(pfd.fd))) {
      err::code = err::LIB_SYS;
      return -1;
    }

    if(bind(pfd.fd, (sockaddr *) &server, sizeof(server)) < 0) {
      err::code = err::LIB_SYS;
      return -1;
    }

    listen(pfd.fd, 1);

    _listenfd = pfd;

    return _listen(f);
  }

  void stop() { _autoRun.stop(); }
  void join() { _autoRun.join(); }

  inline bool isRunning() { return _autoRun.isRunning(); }

private:

  int _listen(std::function<void(Client &&)> _action) {
    int result = 0;

    
    _autoRun.run([&]() {
      if((result = poll(&_listenfd, 1, 100)) > 0) {
        if(_listenfd.revents == POLLIN) {
          DEBUG_LOG("Accepting client");

          auto client = _accept();
          if(client) {
            auto c = util::cmove(*client);
            _task.push([=]() mutable {
              _action(c);
            });
          }
        }
      }
      else if(result == -1) {
	err::code = err::LIB_SYS;
        print(error, "Cannot poll socket: ", err::current());

        _autoRun.stop();
      }
    });

    // Cleanup
    close(_listenfd.fd);
    
    if(result == -1) {
      return -1;
    }
    
    return 0;
  }

  /*
   * User defined methods
   */
  util::Optional<Client> _accept();

  // Generate socket
  int _socket();
};
}
#endif

# Kitty

Kitty is a library designed to ease both making use of and extending functionality
Modules that add dependencies don't build by default

### Module err:
```c++
namespace err {
/*
 * Thread safe error functions
 */
const char *current();

typedef enum {
  OK,
  TIMEOUT,
  BREAK, // Break from eachByte()
  FILE_CLOSED,
  OUT_OF_BOUNDS,
  INPUT_OUTPUT,
  UNAUTHORIZED,
  LIB_GAI,
  LIB_SYS,
  LIB_SSL
} code_t;

extern THREAD_LOCAL util::ThreadLocal<code_t> code;
}
```
LIB_* error codes are for errors that are generated outside Kitty


### Module util
Contains all sorts of nifty features.

####### MoveByCopy

Similar to std::ref, but moves instead.

####### Optional

The following outputs: `enabled`
```c++
util::Optional<std::string> enabled("enabled");
if(enabled) {
  print(fout, op.val, '\n');
}
else {
  print(fout, "disabled\n");
}
```

The following outputs: `disabled`
```c++
util::Optional<std::string> enabled;
if(enabled) {
  print(fout, op.val, '\n');
}
else {
  print(fout, "disabled\n");
}
```

###### task_pool
Push and pop callable objects from a queue.
Push returns a future based on the return type of the callable object.

Pop returns an optional callable object.
```c++
int main {
    TaskPool pool;
    
    std::future<int> x = pool.push([](){
        int x = 5;
        return x;
    });
    
    (*pool)->run();
    
    print(fout, x.get());
    return 0;
}
```

###### thread_pool
Create threads that handle tasks that are put in a queue.
```c++
#include "thread_pool.h"

int main() {
  // Create a thread pool with 1 worker thread
  ThreadPool pool(1);

  // Create a task with a delay of 500 milliseconds
  std::future<int> x = pool.push([](){
    int x = 5;
    return x;
  }, 500);

  // After about half a second, you'll get a result
  x.get();
  
  return 0;
}
```

###### utility
```c++
/* Transform elem into it's hexadecimal notation */
template<class T>
Hex<T> hex(T &elem) {
  return Hex<T>(elem);
}
```

The following outputs: `0x0001`
```c++
#include "utility.h"

int main() {
  uint16_t val = 0x0001;
  print(fout, "0x", util::hex(val), '\n');
  return 0;
}
```

mk_uniq is identical to std::make_unique

###### string

Compensate for the lack of support for std::to_string on Android

###### thread_t

Contains a custom thread class. It has an identical interface as std::thread.

###### set

concat:
    concat two containers
    
map:
    Apply an operation to every element in the container and return the result
    
map_if:
    Similar to map, but the operation must return an optional.
    
move_if:
    moves an object if the function returns true

copy_to:
    Copies the elements to a different type of container
    
move_to:
    Moves the elements to a different type of container
   
split:
    Split an container into multiple containers 
    
any:
    If any element satifies a condition.
    
make_array:
    Create a static array.
    
    

### Module file
`file::FD` is an extendable class that takes a stream as a template argument.

The following is required for streams
```c++
namespace stream {
class io {
  bool _eof;
  int _fd;

public:
  io();

  void operator =(io&& stream);
  void open(/*Any arguments required to open the stream*/);

  /* read
   * buf.size() is the max size
   * The buf is resized to reflect the bytes read
   * unless there is an error
   */
  int operator>>(std::vector<unsigned char>& buf);

  /* write */
  int operator<<(std::vector<unsigned char>& buf);

  bool is_open();
  bool eof();

  void seal();

  int fd();
};
}

typedef FD<stream::io> io;
```

The following would output: `I am printing: 5`
```c++
#include "io_stream.h"

int main() {
  file::io io(-1, STDOUT_FILENO);

  print(io, "I am printing: ", 5, '\n');
}
```

`print` is one of the few functions present in the global namespace

####### tcp

```c++
namespace file {
  io connect(const char *hostname, const char *port);
}
```

### Module log
* `error`  : "Should only be used when errors are not to be recovered from"
* `warning`: "Should be used when minor errors occur"
* `info`   : "Obvious"
* `debug`  : "Should be used only during debug mode"

this object are members of class `FD<stream::Log<stream::io>>`
They are one of the few objects that exist in the global namespace

The following outputs: `[date] Error: Couldn't do it.`
```c++
print(error, "Couldn't do it.")
```

By default it outputs to stdout.
A file for logging can opened by calling `file::log_open(const char *logPath)`

### Module server
An extendable server that handles listening for and accepting clients

###### server
A new server is created by specializing the class Server<Client>
```c++
struct TcpClient {
  typedef /* Any sockaddr type */ _sockaddr;

  /*
    Any meta-data about the client you require
  */
};

typedef Server<TcpClient> tcp;

template<>
util::Optional<tcp::Client> tcp::_accept(Client& client) {
  /* Accept client  */

  return /* client if no errors */;
}

template<>
int tcp::_socket() {
  return socket(AF_INET6, SOCK_STREAM, 0);
}
```

If any additional parameters are required, one could specialize:

```c++
template<typename Client>
struct DefaultType {
  typedef /* Any class */ Type
};
```

To use the server:
```c++
#include "server.h"

void handle_client(server::TcpClient &&client) {
  print(fout, "client: ", client.ip_addr);
}

int main() {
  server::tcp vikingServer;

  uint16_t port = 8080;

  sockaddr_in6 server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin6_family = AF_INET6;
  server_addr.sin6_port = htons(port);

  if (vikingServer.start(server_addr, handle_client) < 0) {
    print(error, "Error during runtime of server: ", err::current(), '\n');
    return -1;
  }

  return 0;
}
```

###### proxy
It's a simple protocol that sends data across a connection over null-terminated byte strings

A call to load takes the form of:
```
load(FD</* any stream */>,
  <string>, MAX_BYTES,
  <int>,
  <vector of strings>, MAX_STRINGS, MAX_BYTES_PER_STRING
);
```
For example:
```c++
std::string _string;
std::vector<std::string> _vec_string

// socket is an instance of FD<>
load(socket,
  _string, MAX_BYTES,
  integer,
  _vec_strings, MAX_STRINGS, MAX_BYTES
);
```
On success the call to load returns 0


### Module ssl
Contains a wrapper for openssl

###### ssl_client
Contains a specialization for Server<Client>
```c++
typedef Server<SslClient> ssl;
```

###### ssl_stream
Contains a specialization for FD<Stream>
```c++
typedef FD<stream::ssl> ssl;
```


###### ssl
```c++
// Needs to be called ones before using any ssl functions
void init();

// Create a Context
// On failure Context.get() returns nullptr
Context init_ctx_server(std::string &caPath, std::string& certPath, std::string& keyPath);
Context init_ctx_client(std::string &caPath, std::string& certPath, std::string& keyPath);

// Accept incoming connection from a client
file::ssl accept(Context &ctx, int fd);

// Connect to a server
file::ssl connect(Context &ctx, const char *hostname, const char* port);
```


### Module bluetooth

###### blue_client
Contains a specialization for Server<Client>
```c++
typedef Server<BlueClient> bluetooth;
```

###### _bluetooth
```c++
struct device {
  bdaddr_t bdaddr;
  int rssi;
  operator std::string();
};

class HCI : public device {
public:
  HCI();
  HCI(const char *bdaddr);
  HCI(HCI &&dev);

  ~HCI();

  void operator =(HCI &&dev);

  device_arr scan(uint8_t timeout);

  /*
   * Data broadcasted. The receiver doesn't need to create a connection.
   */
  int broadcast(uint8_t *data, uint8_t length);
  int broadcast(const std::vector< bt::Uuid > &uuids);
  
  /*
   * Start or stop advertising.
   * name could be a nullptr
   */
  int advertise(bool enable, const char *name);
};
```

###### ble (gatt)
An instance of `Profile` never changes during it's lifetime.
```c++
const bt::Profile profile { {
    bt::Service {
      // UUID
      "1800", {
        bt::Characteristic {
          // UUID
          "2a00",

          // Static data
          {'L', 'o', 'k', 'i'},

          // Readcallback is not necessary since static data is provided
          nullptr,

          // Writecallback is not necessary since bt::Write is not specified
          nullptr,

          // There are no descriptors
          {},

          // Properties of the characterisic
          bt::READ
        },
        bt::Characteristic {
          "2a01",
          {0x80, 0x00},
          nullptr,
          nullptr,
          {},
          bt::READ
        }
      }
    },
    bt::Service {
      // UUID
      "fc1b4070-4bc2-49e6-a7ad-d316fd6f7bce", {
        bt::Characteristic {
          "d3a447af-c540-4afc-b46a-6bf5084871c0",

          // No static data
          {},

          // Necessary since no static data provided and bt::READ is specified
          Readcallback,

          // Necessary since bt::WRITE is specified
          Writecallback, {
            bt::Descriptor {
              // UUID
              "1234",
              {'D', 'a', 't', 'a'},
              bt::READ
            }
          },
          bt::props_t (bt::READ | bt::WRITE)
        }
      }
    }
  }
};
```

Readcallback must be of type `read_func_type` and Writecallback must be of type `write_func_type`
```c++
typedef std::function<util::Optional<std::vector<uint8_t>>(uint16_t, uint16_t, Session*)> read_func_type;
typedef std::function<int(std::vector<uint8_t>&&, Session*)> write_func_type;
```

To use ble:
```c++
bt::HCI hci;

Session : bt::Session {
  int val;
  Session(int val) : val(val) {}
};

void handle_ble_server(server::BlueClient && client) {
  print(info, "client: ", (std::string)client.dev);

  Session session {
    0
  };

  // This is important!!
  client.session = &session;

  // Transfer control to the profile
  profile.main(client);
  
  // Re-enable advertising
  if(hci.advertise(true, "Loki")) {
    print(error, "Couldn't advertise: ", err::current());
  }
}
```

###### Features ble
* Advertisement
  * Start advertising !
    * name  !
    * uuids !

* Services
  * UUID !
  * Characterisitics
    * UUID !
    * properties
      * read !
      * write (without response) !
      * notify (Maybe)
      * broadcast (NO!)
      * Indicate (NO!)
    * secure
      * Read !
      * Write !

    * Descriptors
      * UUID !
      * read !
      * write (NO!)

  * Included Services (NO!)
* error handling !

Special thanks to [bleno](https://github.com/sandeepmistry/bleno).
> Bleno made it a lot easier for me to implement gatt.

### Requirements
* kitty-server
  * pthreads
* kitty-ssl
  * openssl
  * kitty-server
* kitty-bluetooth
  * bluez
  * kitty-server



### Compiling
```Shell
cmake -DCMAKE_BUILD_TYPE=release -DBUILD_SSL=ON -DBUILD_BLUETOOTH=ON src
make
```

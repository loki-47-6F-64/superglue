project(Kitty-Server)

##################################################################
#Source kitty-servers
FILE (GLOB_RECURSE C_SOURCES "./*.c")
FILE (GLOB_RECURSE CPP_SOURCES "./*.cpp")

add_library(kitty-server STATIC ${C_SOURCES} ${CPP_SOURCES})

set_target_properties(kitty-server PROPERTIES
  PUBLIC_HEADER "server.h;proxy.h;tcp_client.h"
)

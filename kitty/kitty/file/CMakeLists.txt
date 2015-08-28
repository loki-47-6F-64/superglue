project(Kitty-File)

##################################################################
#Source kitty-files
FILE (GLOB_RECURSE C_SOURCES "./*.c")
FILE (GLOB_RECURSE CPP_SOURCES "./*.cpp")

add_library(kitty-file STATIC ${C_SOURCES} ${CPP_SOURCES})

set_target_properties(kitty-file PROPERTIES
  PUBLIC_HEADER "file.h;io_stream.h;tcp.h"
)

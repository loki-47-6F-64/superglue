project(Kitty-Log)

##################################################################
#Source Logs
FILE (GLOB_RECURSE C_SOURCES "./*.c")
FILE (GLOB_RECURSE CPP_SOURCES "./*.cpp")

add_library(kitty-log STATIC ${C_SOURCES} ${CPP_SOURCES})

set_target_properties(kitty-log PROPERTIES
  PUBLIC_HEADER "log.h"
)

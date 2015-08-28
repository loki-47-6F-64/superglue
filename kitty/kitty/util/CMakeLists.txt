project(Kitty-Utils)

##################################################################
#Source files
FILE (GLOB_RECURSE C_SOURCES "./*.c")
FILE (GLOB_RECURSE CPP_SOURCES "./*.cpp")

add_library(kitty-utils STATIC ${C_SOURCES} ${CPP_SOURCES})

set_target_properties(kitty-utils PROPERTIES
  PUBLIC_HEADER "move_by_copy.h;optional.h;thread_pool.h;thread_local.h;thread_t.h;utility.h;set.h;string.h;iterator.h;auto_run.h;template_helper.h"
)

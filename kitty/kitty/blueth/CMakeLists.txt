project(Kitty-Bluetooth)

##################################################################
#Source bluetooths
FILE (GLOB_RECURSE C_SOURCES "./*.c")
FILE (GLOB_RECURSE CPP_SOURCES "./*.cpp")


add_library(kitty-bluetooth STATIC ${C_SOURCES} ${CPP_SOURCES})

target_link_libraries(kitty-bluetooth ${BLUEZ_LIBRARIES})
set_target_properties(kitty-bluetooth PROPERTIES
  PUBLIC_HEADER "blueth.h;blue_client.h;ble.h;uuid.h;blue_stream.h"
)

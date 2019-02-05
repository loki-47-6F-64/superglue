project(SuperGlueConsole)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++17")

set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

# rpath makes the linker (When the excecutable is loaded) search for shared libraries in the local directory

include_directories(
  ${HEADER_INCLUDE_DIRS}
  ${KITTY_INCLUDE_DIRS}
)

add_executable(tman ${WRAPPER_SOURCES} ${C_SOURCES} ${CPP_SOURCES} pack.cpp pack.h tman.cpp TMan.h)
# Check for missing symbols during linking
#SET_TARGET_PROPERTIES(tman PROPERTIES LINK_FLAGS "-pie -rpath '\\$ORIGIN'"")
###################################################################
#Dependencies

TARGET_LINK_LIBRARIES(tman
  #/home/loki/.superglue/devices/04245146063900030939/libs/lib/libart.so
  ${TARGET_LIBRARIES}
  ${KITTY_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
)

#install(
#  TARGETS superglue DESTINATION "${LIBS_ROOT}/${TARGET_ABI}"
#  PERMISSIONS
#    OWNER_READ OWNER_WRITE OWNER_EXECUTE
#    GROUP_READ GROUP_EXECUTE
#    WORLD_READ WORLD_EXECUTE
#)

# libc++ must be bundled in the APK
install(
  FILES "${CXX_SHARED}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
  PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE
)

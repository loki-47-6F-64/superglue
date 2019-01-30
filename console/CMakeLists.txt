project(SuperGlueConsole)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++17")

set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

# rpath makes the linker (When the excecutable is loaded) search for shared libraries in the local directory
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie -rpath '\\$ORIGIN'")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O2 -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")


add_executable(tman tman.cpp)

include_directories(
  ${HEADER_INCLUDE_DIRS}
  ${KITTY_INCLUDE_DIRS}
)

if(${TARGET_ABI} STREQUAL "armeabi-v7a")
  set(CXX_SHARED "${CMAKE_CURRENT_SOURCE_DIR}/../output/armeabi-v7a/arm-linux-androideabi/lib/armv7-a/libc++_shared.so")
elseif(${TARGET_ABI} STREQUAL "x86")
  set(CXX_SHARED "${CMAKE_CURRENT_SOURCE_DIR}/../output/x86/i686-linux-android/lib/libc++_shared.so")
endif()


# Check for missing symbols during linking
# SET_TARGET_PROPERTIES(superglue PROPERTIES LINK_FLAGS "-Wl,--no-undefined")
###################################################################
#Dependencies
TARGET_LINK_LIBRARIES(tman
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

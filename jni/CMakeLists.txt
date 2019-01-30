project(SuperGlueJNI)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++17")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O2 -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")


set (LIBS_ROOT "./app/src/main/jniLibs")

FILE (GLOB_RECURSE C_SOURCES "./*.c")
FILE (GLOB_RECURSE CPP_SOURCES "./*.cpp")
list(APPEND CPP_SOURCES_DJINNI "${CMAKE_SOURCE_DIR}/djinni/support-lib/jni/djinni_support.cpp")

add_library (superglue SHARED ${C_SOURCES} ${CPP_SOURCES} ${CPP_SOURCES_DJINNI})

include_directories(
  ${KITTY_INCLUDE_DIRS}
  "${CMAKE_SOURCE_DIR}/djinni/support-lib/jni"
  "${CMAKE_SOURCE_DIR}/src/generated-src"
  ${CMAKE_CURRENT_SOURCE_DIR}
  "${CMAKE_CURRENT_SOURCE_DIR}/generated-src"
)

if(${TARGET_ABI} STREQUAL "armeabi-v7a")
  set(CXX_SHARED "${CMAKE_CURRENT_SOURCE_DIR}/../output/armeabi-v7a/arm-linux-androideabi/lib/armv7-a/libc++_shared.so")
elseif(${TARGET_ABI} STREQUAL "x86")
  set(CXX_SHARED "${CMAKE_CURRENT_SOURCE_DIR}/../output/x86/i686-linux-android/lib/libc++_shared.so")
endif()


# Check for missing symbols during linking
SET_TARGET_PROPERTIES(superglue PROPERTIES LINK_FLAGS "-Wl,--no-undefined")
###################################################################
#Dependencies
TARGET_LINK_LIBRARIES(superglue
  superglueC
)

install(
  TARGETS superglue DESTINATION "${LIBS_ROOT}/${TARGET_ABI}"
  PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE
)

# libc++ must be bundled in the APK
install(
  FILES "${CXX_SHARED}" DESTINATION "${LIBS_ROOT}/${TARGET_ABI}"
  PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE
)

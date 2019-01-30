cmake_minimum_required(VERSION 2.8)

if(BUILD_EXTERNAL_MULTI_ARCH)
  # No need for compilers in this mode
  project(SuperGlue NONE)
else()
  project(SuperGlue)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ExternalProject)

# Download and build external projects.
include(BuildExternal)

if(NOT BUILD_EXTERNAL_MULTI_ARCH)
  if(${TARGET_PLATFORM} STREQUAL IOS)
    add_definitions( -DLACKS_FEATURE_THREAD_LOCAL )
    set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libs/${IOS_PLATFORM})
  
    add_subdirectory(kitty)
  
    add_subdirectory(src)
    add_subdirectory(objc)

  elseif(${TARGET_PLATFORM} STREQUAL ANDROID)
    if(NOT DEFINED TOOLCHAIN_ROOT)
      message(FATAL_ERROR "TOOLCHAIN_ROOT is not defined")
    endif()
  
    if(NOT DEFINED TARGET_ABI)
      message(FATAL_ERROR "TARGET_ABI is not defined")
    endif()
  
    if(${TARGET_ABI} STREQUAL "armeabi-v7a") 
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv7-a")
      set(CXX_SHARED "${CMAKE_CURRENT_SOURCE_DIR}/../output/armeabi-v7a/arm-linux-androideabi/lib/armv7-a/libc++_shared.so")
    elseif(${TARGET_ABI} STREQUAL "x86")
      set(CXX_SHARED "${CMAKE_CURRENT_SOURCE_DIR}/../output/x86/i686-linux-android/lib/libc++_shared.so")
    endif()

    if(${TARGET_ABI} STREQUAL "armeabi-v7a")
          endif()
  
    add_subdirectory(kitty)
  
    find_package(Threads REQUIRED)

    if(NOT BUILD_CONSOLE_APP)
      add_subdirectory(src)
      add_subdirectory(jni)
    else()
      add_subdirectory(console)
    endif()
  else()
    message(FATAL_ERROR "Unknown platform: ${TARGET_PLATFORM}")
  endif()
endif()

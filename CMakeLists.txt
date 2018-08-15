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
  
    set(KITTY_BUILD_SSL ON)
    add_subdirectory(kitty)
  
    find_package(Kitty
      REQUIRED
      PATHS "${PROJECT_BINARY_DIR}/kitty"
      NO_DEFAULT_PATH
    )
  
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
    endif()
  
    add_subdirectory(kitty)
    find_package(Kitty
      REQUIRED
      PATHS "${PROJECT_BINARY_DIR}/kitty"
      NO_DEFAULT_PATH
    )
  
    find_package(Threads REQUIRED)
    add_subdirectory(src)
    add_subdirectory(jni)
  else()
    message(FATAL_ERROR "Unknown platform: ${TARGET_PLATFORM}")
  endif()
endif()

if(DUMMY_CMAKE)
  return()
endif()

set(DUMMY_CMAKE 1)


set(binary_dir ${EXTERNAL_PROJECT_BINARY_DIR}/Dummy-prefix/src/Dummy)
add_library(download-dummy STATIC IMPORTED)

set(DUMMY_LIBRARIES download-dummy)

if(${TARGET_PLATFORM} STREQUAL ANDROID)
  # Some build-scripts build only a single architecture at a time.
  if(NOT BUILD_EXTERNAL_MULTI_ARCH)
    # Do something specific for the TARGET_ABI
    if(${TARGET_ABI} STREQUAL "armeabi-v7a")
    elseif(${TARGET_ABI} STREQUAL "x86")
    else()
      message(FATAL_ERROR "Dummy: Unsupported target abi")
    endif()
    
    set(TOOLCHAIN_DIR "${PROJECT_SOURCE_DIR}/output/${TARGET_ABI}")

    # Default RANLIB of OSX is not compatible with Android
    file(GLOB CC "${TOOLCHAIN_DIR}/bin/*-gcc")

    # Extract name host compiler
    get_filename_component(HOST_COMPILER_CC ${CC} NAME)
    string(REPLACE "-gcc" "" ${HOST_COMPILER_CC} HOST_COMPILER)

    set(CXX "${TOOLCHAIN_DIR}/bin/${HOST_COMPILER}-g++")
    set(RANLIB "${TOOLCHAIN_DIR}/bin/${HOST_COMPILER}-ranlib")
    set(AR "${TOOLCHAIN_DIR}/bin/${HOST_COMPILER}-ar")
    set(AS "${TOOLCHAIN_DIR}/bin/${HOST_COMPILER}-as")
    set(LD "${TOOLCHAIN_DIR}/bin/${HOST_COMPILER}-ld")
    set(NM "${TOOLCHAIN_DIR}/bin/${HOST_COMPILER}-nm")
    
    ExternalProject_Add(Dummy
      BUILD_IN_SOURCE 1
      GIT_REPOSITORY ""
      GIT_TAG "master"
      BUILD_COMMAND ""
      UPDATE_COMMAND ""
      INSTALL_COMMAND ""
      CONFIGURE_COMMAND ""
    )

    add_dependencies(download-dummy Dummy)

    # This is only necessary for DYNAMIC libraries on Android
    list(APPEND INSTALL_LIBS ${DUMMY_LIBRARIES})
  endif()
elseif(${TARGET_PLATFORM} STREQUAL IOS)
  # Some build-scripts build the library for all architectures instead of one at a time.
  if(BUILD_EXTERNAL_MULTI_ARCH)
    ExternalProject_Add(Dummy
      # Xcode changes some environment varaiables. "env" "-i" "sh" "-c" temporarily resets the environment. 
      BUILD_COMMAND "env" "-i" "sh" "-c" "./build-script.sh"
      BUILD_IN_SOURCE 1
    )

    # Tell cmake wich libraries were build or downloaded in the build-script
    list(APPEND INSTALL_LIBS ${DUMMY_LIBRARIES})
  endif()
else()
  message(FATAL_ERROR "Unknown platform ${TARGET_PLATFORM}")
endif()

# Let cmake know where to find the library
set_property(TARGET download-dummy PROPERTY IMPORTED_LOCATION "${binary_dir}/libdummy.a")

set(DUMMY_INCLUDE_DIRS "${binary_dir}/dummy/include")

# If libdummy needs some wrappers
file(GLOB_RECURSE WRAPPER_SOURCES_DUMMY "${CMAKE_CURRENT_SOURCE_DIR}/wrappers/dummy/*.cpp")
list(APPEND WRAPPER_SOURCES ${WRAPPER_SOURCES_DUMMY})
list(APPEND DUMMY_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/wrappers/dummy)

# Let src/CMakeLists.txt know what dependecies to include
list(APPEND HEADER_INCLUDE_DIRS ${DUMMY_INCLUDE_DIRS})
list(APPEND TARGET_LIBRARIES ${DUMMY_LIBRARIES})

if(PJ_CMAKE)
  return()
endif()

set(PJ_CMAKE 1)


add_library(download-pjlib STATIC IMPORTED)
add_library(download-pjlib-util STATIC IMPORTED)
add_library(download-pjnath STATIC IMPORTED)

set(PJSIP_LIBRARIES download-pjnath download-pjlib-util download-pjlib)

if(${TARGET_PLATFORM} STREQUAL ANDROID)
  # Some build-scripts build only a single architecture at a time.
  if(NOT BUILD_EXTERNAL_MULTI_ARCH)
    # Do something specific for the TARGET_ABI
    if(${TARGET_ABI} STREQUAL "armeabi-v7a")
	set(MACHINE_NAME arm-unknown-linux-android)
    elseif(${TARGET_ABI} STREQUAL "x86")
	set(MACHINE_NAME i686-pc-linux-android)
      elseif(${TARGET_ABI} STREQUAL "arm64-v8a")
        set(MACHINE_NAME aarch64-unknown-linux-android)
    else()
      message(FATAL_ERROR "pjsip: Unsupported target abi")
    endif()

    ExternalProject_Add(PJSIP
      BUILD_IN_SOURCE 1
      GIT_REPOSITORY "https://github.com/pjsip/pjproject.git"
      GIT_TAG "master"
      BUILD_COMMAND
	"make"
      UPDATE_COMMAND ""
      INSTALL_COMMAND ""
      PATCH_COMMAND patch < "${PATCH_DIR}/configure-android.patch"
      CONFIGURE_COMMAND
      "CFLAGS=-g" "ANDROID_NDK_ROOT=${ANDROID_NDK_ROOT}" "TARGET_ABI=${TARGET_ABI}" "APP_PLATFORM=android-${TARGET_PLATFORM_NR}" "./configure-android"
      COMMAND "make" "dep"
    )

    ExternalProject_Get_Property(PJSIP binary_dir)

    ExternalProject_Add_Step(PJSIP config-site
      COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/cmake/patch/pj_config_site.h ${binary_dir}/pjlib/include/pj/config_site.h
      COMMAND patch "-d${binary_dir}/pjlib/include/pj/" < "${PATCH_DIR}/pj_config.patch"
      DEPENDERS configure
      DEPENDEES patch
      )
    add_dependencies(download-pjlib PJSIP)
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

message("${binary_dir}/pjnath/lib/libpjnath-${MACHINE_NAME}.a")
# Let cmake know where to find the library
set_property(TARGET download-pjnath PROPERTY IMPORTED_LOCATION "${binary_dir}/pjnath/lib/libpjnath-${MACHINE_NAME}.a")
set_property(TARGET download-pjlib-util PROPERTY IMPORTED_LOCATION "${binary_dir}/pjlib-util/lib/libpjlib-util-${MACHINE_NAME}.a")
set_property(TARGET download-pjlib PROPERTY IMPORTED_LOCATION "${binary_dir}/pjlib/lib/libpj-${MACHINE_NAME}.a")

set(PJSIP_INCLUDE_DIRS
	"${binary_dir}/pjlib/include"
	"${binary_dir}/pjlib-util/include"
	"${binary_dir}/pjnath/include"
)

add_dependencies(download-pjlib-util download-pjlib)
add_dependencies(download-pjnath download-pjlib-util)

file(GLOB_RECURSE WRAPPER_SOURCES_PJSIP
  "${CMAKE_CURRENT_SOURCE_DIR}/wrappers/pjsip/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/wrappers/pjsip/*.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/wrappers/pjsip/*.h"
  )
list(APPEND WRAPPER_SOURCES ${WRAPPER_SOURCES_PJSIP})
list(APPEND PJSIP_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/wrappers/pjsip)

# Let src/CMakeLists.txt know what dependecies to include
list(APPEND HEADER_INCLUDE_DIRS ${PJSIP_INCLUDE_DIRS})
list(APPEND TARGET_LIBRARIES ${PJSIP_LIBRARIES})

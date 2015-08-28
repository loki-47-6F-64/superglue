if(CURL_CMAKE)
  return()
endif()

set(CURL_CMAKE 1)


add_library(download-curl STATIC IMPORTED)

set(CURL_LIBRARIES download-curl -lz)

if(${TARGET_PLATFORM} STREQUAL ANDROID)
  # Some build-scripts build only a single architecture at a time.
  if(NOT BUILD_EXTERNAL_MULTI_ARCH)
    set(TOOLCHAIN_DIR "${PROJECT_SOURCE_DIR}/output/${TARGET_ABI}")

    # Default RANLIB of OSX is not compatible with Android
    file(GLOB CC "${TOOLCHAIN_DIR}/bin/*-gcc")

    get_filename_component(HOST_COMPILER_CC ${CC} NAME)
    string(REPLACE "-gcc" "" ${HOST_COMPILER_CC} HOST_COMPILER)

    file(GLOB CXX "${TOOLCHAIN_DIR}/bin/*-g++")
    file(GLOB RANLIB "${TOOLCHAIN_DIR}/bin/${HOST_COMPILER}-ranlib")
    file(GLOB AR "${TOOLCHAIN_DIR}/bin/${HOST_COMPILER}-ar")
    file(GLOB AS "${TOOLCHAIN_DIR}/bin/${HOST_COMPILER}-as")
    file(GLOB LD "${TOOLCHAIN_DIR}/bin/${HOST_COMPILER}-ld")
    file(GLOB NM "${TOOLCHAIN_DIR}/bin/${HOST_COMPILER}-nm")
    
    set(PREFIX "${CMAKE_CURRENT_BINARY_DIR}/curl_build/")
    ExternalProject_Add(CURL
      DEPENDS ${OPENSSL_LIBRARIES}
      URL "http://curl.haxx.se/download/curl-7.42.1.tar.gz"
      #URL "http://curl.haxx.se/download/curl-7.37.1.tar.gz"
      CONFIGURE_COMMAND
        "./buildconf"
      COMMAND
        "CPPFLAGS=-I${OPENSSL_INCLUDE_DIRS}" "LDFLAGS=-L${OPENSSL_INCLUDE_DIRS}/../"
        "CC=${CC}" "RANLIB=${RANLIB}" "AR=${AR}" "AS=${AS}" "LD=${LD}"
        "NM=${NM}" "./configure" "--prefix=${PREFIX}" "--host=${HOST_COMPILER}" "--target=${HOST_COMPILER}" "--with-ssl" "--with-zlib"
      BUILD_COMMAND "make"
      INSTALL_COMMAND "make" "install"
      BUILD_IN_SOURCE 1
    )

    # Let cmake find the binary_dir
    ExternalProject_Get_Property(CURL binary_dir)
    add_dependencies(download-curl CURL)
  endif()
elseif(${TARGET_PLATFORM} STREQUAL IOS)
  set(binary_dir ${EXTERNAL_PROJECT_BINARY_DIR}/CURL-prefix/src/CURL)
  set(CURL_LIBRARIES download-curl)
  # Some build-scripts build the library for all architectures instead of one at a time.
  # This is to make sure the build-script isn't run unnecessarily.
  if(BUILD_EXTERNAL_MULTI_ARCH)
    ExternalProject_Add(CURL
      GIT_REPOSITORY "https://github.com/brunodecarvalho/curl-ios-build-scripts.git"
      GIT_TAG master
      CONFIGURE_COMMAND ""
      INSTALL_COMMAND ""
      UPDATE_COMMAND ""
      PATCH_COMMAND patch -p0 < "${PATCH_DIR}/curl.patch"
      BUILD_IN_SOURCE 1
      # Xcode changes some environment varaiables. "env" "-i" "sh" "-c" temporarily resets the environment. 
      BUILD_COMMAND "env" "-i" "sh" "-c" "./build_curl --osx-sdk-version none --sdk-version=''"
    )

    # Tell cmake wich libraries were build or downloaded in the build-script
    list(APPEND INSTALL_LIBS ${CURL_LIBRARIES})

    add_dependencies(download-curl CURL)
  endif()

  set(PREFIX "${binary_dir}/curl/ios-dev")
else()
  message(FATAL_ERROR "Unknown platform ${TARGET_PLATFORM}")
endif()

# Let cmake know where to find the library
set_property(TARGET download-curl PROPERTY IMPORTED_LOCATION "${PREFIX}/lib/libcurl.a")

set(CURL_INCLUDE_DIRS "${PREFIX}/include")

# If libdummy needs some wrappers
file(GLOB_RECURSE WRAPPER_SOURCES_CURL "${CMAKE_CURRENT_SOURCE_DIR}/wrappers/curl/*.cpp")
list(APPEND WRAPPER_SOURCES ${WRAPPER_SOURCES_CURL})
list(APPEND CURL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/wrappers/curl)

list(APPEND HEADER_INCLUDE_DIRS ${CURL_INCLUDE_DIRS})
list(APPEND TARGET_LIBRARIES ${CURL_LIBRARIES})

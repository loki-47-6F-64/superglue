if(OPENSSL_CMAKE)
  return()
endif()

set(OPENSSL_CMAKE 1)


add_library(download-crypto STATIC IMPORTED)
add_library(download-ssl STATIC IMPORTED)

set(OPENSSL_LIBRARIES download-ssl download-crypto)


if(${TARGET_PLATFORM} STREQUAL ANDROID)
  if(NOT BUILD_EXTERNAL_MULTI_ARCH)
    if(${TARGET_ABI} STREQUAL "armeabi-v7a")
      set(CONFIGURE_ABI "android-armv7")
    elseif(${TARGET_ABI} STREQUAL "x86")
      set(CONFIGURE_ABI "android-x86")
    else()
      message(FATAL_ERROR "OpenSSL: Unsupported target arch")
    endif()
    
    # Default RANLIB of OSX is not compatible with Android
    file(GLOB CC "${PROJECT_SOURCE_DIR}/output/${TARGET_ABI}/bin/*-gcc")
    file(GLOB RANLIB "${CC}-ranlib")
    file(GLOB AR "${CC}-ar")
    file(GLOB NM "${CC}-nm")
    file(GLOB CC "${PROJECT_SOURCE_DIR}/output/${TARGET_ABI}/bin/*-gcc")
    
    ExternalProject_Add(OpenSSL
      GIT_REPOSITORY "https://github.com/openssl/openssl.git"
      GIT_TAG "OpenSSL_1_0_1-stable"
    
      UPDATE_COMMAND "git" "pull"
      CONFIGURE_COMMAND "CC=${CC}" "RANLIB=${RANLIB}" "AR=${AR}" "NM=${NM}" "./Configure" "${CONFIGURE_ABI}"
      BUILD_IN_SOURCE 1
      BUILD_COMMAND "ANDROID_DEV=${PROJECT_SOURCE_DIR}/output/${TARGET_ABI}/sysroot/usr" "make" "build_libs"
      INSTALL_COMMAND ""
    )

    ExternalProject_Get_Property(OpenSSL binary_dir)
    add_dependencies(download-crypto OpenSSL)
  endif()
elseif(${TARGET_PLATFORM} STREQUAL IOS)
  set(binary_dir ${EXTERNAL_PROJECT_BINARY_DIR}/OpenSSL-prefix/src/Openssl)

  if(BUILD_EXTERNAL_MULTI_ARCH)
    ExternalProject_Add(OpenSSL
      GIT_REPOSITORY "https://github.com/x2on/OpenSSL-for-iPhone.git"
      GIT_TAG "master"

      UPDATE_COMMAND "git" "pull"
      BUILD_IN_SOURCE 1
      CONFIGURE_COMMAND ""

      # Xcode environment causes undefined symboles in build script
      BUILD_COMMAND "env" "-i" "sh" "-c" "./build-libssl.sh"

      INSTALL_COMMAND ""
    )

    list(APPEND INSTALL_LIBS ${OPENSSL_LIBRARIES})
  endif()
  set(OPENSSL_LIB_DIR "lib")
else()
  message(FATAL_ERROR "Unknown platform ${TARGET_PLATFORM}")
endif()

set_property(TARGET download-crypto PROPERTY IMPORTED_LOCATION "${binary_dir}/${OPENSSL_LIB_DIR}/libcrypto.a")
set_property(TARGET download-ssl PROPERTY IMPORTED_LOCATION "${binary_dir}/${OPENSSL_LIB_DIR}/libssl.a")

set(OPENSSL_INCLUDE_DIR "${binary_dir}/include")
set(OPENSSL_INCLUDE_DIRS "${binary_dir}/include")

add_dependencies(download-ssl download-crypto)

set(CMAKE_DISABLE_FIND_PACKAGE_OpenSSL TRUE)

list(APPEND HEADER_INCLUDE_DIRS ${OPENSSL_INCLUDE_DIRS})
list(APPEND TARGET_LIBRARIES ${OPENSSL_LIBRARIES})

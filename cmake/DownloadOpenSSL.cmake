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
      set(CONFIGURE_ABI "android-armeabi")
    elseif(${TARGET_ABI} STREQUAL "x86")
      set(CONFIGURE_ABI "android-x86")
    elseif(${TARGET_ABI} STREQUAL "arm64-v8a")
      set(CONFIGURE_ABI "android64-aarch64")
    else()
      message(FATAL_ERROR "OpenSSL: Unsupported target arch")
    endif()
    
    ExternalProject_Add(OpenSSL
      URL https://www.openssl.org/source/openssl-1.1.0h.tar.gz
      URL_HASH SHA1=0fc39f6aa91b6e7f4d05018f7c5e991e1d2491fd
    
      UPDATE_COMMAND ""
      PATCH_COMMAND
        sed -i.back -e "s/\\-mandroid//g" Configurations/10-main.conf
      CONFIGURE_COMMAND
      "CC=${CC}" "RANLIB=${RANLIB}" "AR=${AR}" "NM=${NM}" "./Configure" "${CONFIGURE_ABI}" "-I${CMAKE_FIND_ROOT_PATH}/include/${NDK_BINARY_PREFIX}" "-L${CMAKE_FIND_ROOT_PATH}/lib/${NDK_BINARY_PREFIX}/${TARGET_PLATFORM_NR}"

      BUILD_IN_SOURCE 1
      BUILD_COMMAND
      "CROSS_SYSROOT=${CMAKE_FIND_ROOT_PATH}" "make" "build_libs"
      INSTALL_COMMAND ""
    )

    ExternalProject_Add_Step(OpenSSL clean-sed
      COMMAND ${CMAKE_COMMAND} -E remove Configurations/10-main.conf.back
      DEPENDERS configure
      DEPENDEES patch
    )

    ExternalProject_Get_Property(OpenSSL binary_dir)
    
    set(COPY_FROM_DIR ${CMAKE_FIND_ROOT_PATH}/lib/${NDK_BINARY_PREFIX}/${TARGET_PLATFORM_NR})
    ExternalProject_Add_Step(OpenSSL link-to-crt_so
      COMMAND ${CMAKE_COMMAND} -E copy ${COPY_FROM_DIR}/crtbegin_so.o ${COPY_FROM_DIR}/crtend_so.o ${binary_dir}
      DEPENDERS build
      DEPENDEES configure
      )

      add_dependencies(download-crypto OpenSSL)
  endif()
elseif(${TARGET_PLATFORM} STREQUAL IOS)
  set(binary_dir ${EXTERNAL_PROJECT_BINARY_DIR}/OpenSSL-prefix/src/Openssl)

  if(BUILD_EXTERNAL_MULTI_ARCH)
    ExternalProject_Add(OpenSSL
      GIT_REPOSITORY "https://github.com/x2on/OpenSSL-for-iPhone.git"
      GIT_TAG "master"

      UPDATE_COMMAND ""
      BUILD_IN_SOURCE 1
      CONFIGURE_COMMAND ""

      # Xcode environment causes undefined symboles in build script
      #BUILD_COMMAND "env" "-i" "sh" "-c" "./build-libssl.sh" "--version=1.1.0h" "--targets=ios-sim-cross-x86_64 ios64-cross-arm64 ios-cross-armv7"
      BUILD_COMMAND "./build-libssl.sh" "--version=1.1.0h" "--targets=ios-sim-cross-x86_64 ios64-cross-arm64 ios-cross-armv7s ios-cross-armv7"

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

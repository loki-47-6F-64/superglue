#if(NOT DEFINED TOOLCHAIN_ROOT)
#  message(FATAL_ERROR "TOOLCHAIN_ROOT is not defined")
#endif()
#
#if(NOT DEFINED TARGET_ABI)
#  message(FATAL_ERROR "TARGET_ABI is not defined")
#endif()

set(TARGET_PLATFORM_NR "21" CACHE STRING "")
set(TARGET_ABI "" CACHE STRING "")

file(GLOB TOOLCHAIN_ROOT "${TOOLCHAIN_ROOT}")
file(GLOB ANDROID_NDK_ROOT ${TOOLCHAIN_ROOT}/toolchain/android-ndk-*)

# file(GLOB TARGET_ROOT ${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/*-x86_64)
set(TARGET_ROOT ${TOOLCHAIN_ROOT}/${TARGET_ABI})

set(COMPILER_DIR ${TARGET_ROOT}/bin)

message("TOOLCHAIN_ROOT == ${TOOLCHAIN_ROOT}")
message("ANDROID_NDK_ROOT == ${ANDROID_NDK_ROOT}")
message("TARGET_ROOT == ${TARGET_ROOT}")
message("COMPILER_DIR == ${COMPILER_DIR}")

if(${TARGET_ABI} STREQUAL "armeabi-v7a")
  set(NDK_BINARY_PREFIX "arm-linux-androideabi")
  set(NDK_COMPILER_TARGET "armv7a-none-linux-androideabi")
  set(TARGET_ABI_FLAGS "-mfpu=vfpv3-d16 -mthumb -mfpu=neon")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--fix-cortex-a8")
elseif(${TARGET_ABI} STREQUAL "arm64-v8a")
  set(NDK_BINARY_PREFIX "aarch64-linux-android")
  set(NDK_COMPILER_TARGET "aarch64-none-linux-androideabi")
elseif(${TARGET_ABI} STREQUAL "x86")
  set(NDK_BINARY_PREFIX "i686-linux-android")
  set(NDK_COMPILER_TARGET "i686-none-linux-androideabi")  
elseif(${TARGET_ABI} STREQUAL "x86_64")
  set(NDK_BINARY_PREFIX "x86_64-linux-android")
  set(NDK_COMPILER_TARGET "x86_64-none-linux-androideabi")
else()
  message(FATAL_ERROR "Invalid TARGET_ABI ${TARGET_ABI}")
endif()

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -target ${NDK_COMPILER_TARGET}${TARGET_PLATFORM_NR} ${TARGET_ABI_FLAGS} -Wl,--gc-sections -Wl,--export-dynamic -pie -fPIE -Wl,--build-id -latomic -Wl,--no-undefined -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now -Wl,--warn-shared-textrel -Wl,--fatal-warnings -lm -rpath \"\$ORIGIN\"")
set(ANDROID_FLAGS "-ffunction-sections -fstack-protector-strong -funwind-tables -no-canonical-prefixes -target ${NDK_COMPILER_TARGET}${TARGET_PLATFORM_NR} ${TARGET_ABI_FLAGS} -D__ANDROID_API__=${TARGET_PLATFORM_NR} -Wno-invalid-command-line-argument -Wno-unused-command-line-argument  -fno-addrsig -fpic -DANDROID -pie -fPIE -Wa,--noexecstack -Wformat -Werror=format-security")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ANDROID_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ANDROID_FLAGS}")

FILE(GLOB_RECURSE CMAKE_C_COMPILER "${COMPILER_DIR}/clang")
FILE(GLOB_RECURSE CMAKE_CXX_COMPILER "${COMPILER_DIR}/clang++")

# where is the target environment 
SET(CMAKE_FIND_ROOT_PATH  ${TARGET_ROOT}/sysroot/usr)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CXX_SHARED ${CMAKE_FIND_ROOT_PATH}/lib/${NDK_BINARY_PREFIX}/libc++_shared.so)

set(RANLIB "${COMPILER_DIR}/${NDK_BINARY_PREFIX}-ranlib")
set(AR  "${COMPILER_DIR}/llvm-ar")
set(NM  "${COMPILER_DIR}/llvm-nm")
set(CC  "${CMAKE_C_COMPILER}")
set(CXX "${CMAKE_CXX_COMPILER}")

message("RANLIB == ${RANLIB}")
message("AR == ${AR}")
message("NM == ${NM}")
message("CC == ${CC}")
message("CXX == ${CXX}")
message("CMAKE_FIND_ROOT_PATH == ${CMAKE_FIND_ROOT_PATH}")

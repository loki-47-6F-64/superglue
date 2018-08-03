# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)

#if(NOT DEFINED TOOLCHAIN_ROOT)
#  message(FATAL_ERROR "TOOLCHAIN_ROOT is not defined")
#endif()
#
#if(NOT DEFINED TARGET_ABI)
#  message(FATAL_ERROR "TARGET_ABI is not defined")
#endif()

SET(TARGET_ROOT "${TOOLCHAIN_ROOT}/${TARGET_ABI}")
SET(COMPILER_DIR ${TARGET_ROOT}/bin)

# specify the cross compiler

FILE(GLOB_RECURSE CMAKE_C_COMPILER "${COMPILER_DIR}/*-clang")
FILE(GLOB_RECURSE CMAKE_CXX_COMPILER "${COMPILER_DIR}/*-clang++")

# where is the target environment 
SET(CMAKE_FIND_ROOT_PATH  ${TARGET_ROOT}/sysroot/usr)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

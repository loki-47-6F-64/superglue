project(SuperGlueSource)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-missing-braces -std=c++14")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

##################################################################
#Source files
FILE (GLOB_RECURSE C_SOURCES "*.c")
FILE (GLOB_RECURSE CPP_SOURCES "*.cpp")

INCLUDE_DIRECTORIES (
  ${HEADER_INCLUDE_DIRS}
  ${KITTY_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/generated-src
)

add_library(superglueC STATIC ${C_SOURCES} ${CPP_SOURCES} ${WRAPPER_SOURCES})

add_dependencies(superglueC ${TARGET_DEPENDENCIES})
TARGET_LINK_LIBRARIES(superglueC
  ${TARGET_LIBRARIES}
  ${KITTY_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
)

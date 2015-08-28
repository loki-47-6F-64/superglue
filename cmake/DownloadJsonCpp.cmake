if(JSONCPP_CMAKE)
  return()
endif()

set(JSONCPP_CMAKE 1)


set(JSONCPP_LIBRARIES download-jsoncpp)
set(binary_dir ${EXTERNAL_PROJECT_BINARY_DIR}/Jsoncpp-prefix/src/Jsoncpp)

# Some build-scripts build only a single architecture at a time.
if(BUILD_EXTERNAL_MULTI_ARCH)
  ExternalProject_Add(Jsoncpp
    GIT_REPOSITORY "https://github.com/open-source-parsers/jsoncpp.git"
    GIT_TAG "master"
    BUILD_COMMAND "python" "amalgamate.py"
    UPDATE_COMMAND ""
    INSTALL_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_IN_SOURCE 1
  )
else()
  add_library(download-jsoncpp STATIC "${binary_dir}/dist/jsoncpp.cpp")
endif()

set(JSONCPP_INCLUDE_DIRS "${binary_dir}/dist")

# Let src/CMakeLists.txt know what dependecies to include
list(APPEND HEADER_INCLUDE_DIRS ${JSONCPP_INCLUDE_DIRS})
list(APPEND TARGET_LIBRARIES ${JSONCPP_LIBRARIES})

if(DUMMY_CMAKE)
  return()
endif()

set(DUMMY_CMAKE 1)


set(DUMMY_LIBRARIES Dummy)

# Some build-scripts build only a single architecture at a time.
if(NOT BUILD_EXTERNAL_MULTI_ARCH)
  ExternalProject_Add(Dummy
    GIT_REPOSITORY "https://github.com/dummy/dummy.git"
    GIT_TAG "dummy"
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""
    BUILD_COMMAND ""
    BUILD_IN_SOURCE 1
  )

  # Let cmake find the binary_dir
  ExternalProject_Get_Property(Dummy binary_dir)
  set(DUMMY_INCLUDE_DIRS "${binary_dir}/include")
endif()

list(APPEND HEADER_INCLUDE_DIRS ${DUMMY_INCLUDE_DIRS})
list(APPEND TARGET_DEPENDENCIES ${DUMMY_LIBRARIES})

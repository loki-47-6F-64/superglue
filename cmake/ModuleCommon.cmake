if(COMMON_MODULE_CMAKE)
  return()
endif()

set(COMMON_MODULE_CMAKE 1)

if(${TARGET_PLATFORM} STREQUAL ANDROID)
  find_module(superglue)
endif()
find_module(common)

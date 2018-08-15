if(BLUECAST_MODULE_CMAKE)
  return()
endif()

set(BLUECAST_MODULE_CMAKE 1)

include(ModuleCommon)

find_module(bluecast)

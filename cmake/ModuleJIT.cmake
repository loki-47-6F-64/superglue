if(JIT_MODULE_CMAKE)
  return()
endif()

set(JIT_MODULE_CMAKE 1)

include(ModuleCommon)

find_module(jit)

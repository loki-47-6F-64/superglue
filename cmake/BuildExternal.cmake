function(file_download URL DESTINATION)
  if(EXISTS "${DESTINATION}")
    message("[${DESTINATION}] already exists -- skipping download")
  else()
    file(
      DOWNLOAD ${URL} ${DESTINATION}
      STATUS STATUS_VAR
      SHOW_PROGRESS
    )

    list(GET STATUS_VAR 0 ERR_CODE)
    if(ERR_CODE)
      list(GET STATUS_VAR 1 ERR_MSG)
      message(FATAL_ERROR "Could not download [${URL}] -- ${ERR_MSG}")
    endif()
  endif()
endfunction()

macro(find_module MODULE)
  set(MODULE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/${MODULE}")

  list(APPEND MODULE_DIRS ${MODULE_DIR})
  if(${TARGET_PLATFORM} STREQUAL ANDROID)
    file(GLOB JNI_SOURCES "${MODULE_DIR}/native/jni/*.cpp")

    list(APPEND CPP_SOURCES_DJINNI ${JNI_SOURCES})
  endif()

  file(GLOB _DJINNI_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/output "${MODULE_DIR}/djinni/*.djinni")
  file(GLOB SOURCES "${MODULE_DIR}/native/cpp/*.cpp")
  file(GLOB _DJINNI_SOURCES_ABS "${MODULE_DIR}/djinni/*.djinni")
  list(APPEND DJINNI_SOURCES_ABS ${_DJINNI_SOURCES_ABS})
  
  list(APPEND WRAPPER_SOURCES ${SOURCES})
  list(APPEND DJINNI_SOURCES ${_DJINNI_SOURCES})

  list(APPEND HEADER_INCLUDE_DIRS "${MODULE_DIR}/native/cpp")
endmacro()

if(${TARGET_PLATFORM} STREQUAL IOS)
  ### Create symbolic link to modules
  ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_INSTALL_PREFIX}/superglue
    COMMAND ln -sv ${CMAKE_SOURCE_DIR}/modules ${CMAKE_INSTALL_PREFIX}/superglue
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
  
  ADD_CUSTOM_TARGET(symbolic_install ALL
    DEPENDS ${CMAKE_INSTALL_PREFIX}/superglue
    SOURCES ${DJINNI_SOURCES_ABS}
  )
endif()

set(PATCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake/patch")
if(${TARGET_PLATFORM} STREQUAL IOS)
  # Finding git fails on ios

  if(NOT DEFINED EXTERNAL_PROJECT_BINARY_DIR)
    set(EXTERNAL_PROJECT_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build/ios/external)
  endif()

  set(INSTALL_DESTINATION superglue/libs)
elseif(${TARGET_PLATFORM} STREQUAL ANDROID)
  if(NOT DEFINED EXTERNAL_PROJECT_BINARY_DIR)
    if(BUILD_CONSOLE_APP)
      set(EXTERNAL_PROJECT_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build/console/external)
    else()
      set(EXTERNAL_PROJECT_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build/android/external)
    endif()
  endif()

  set(INSTALL_DESTINATION app/src/main/jniLibs/${TARGET_ABI})
else()
  message(FATAL_ERROR "Unknown platform: ${TARGET_PLATFORM}")
endif()

# If building for a console application, don't add install dependencies
if(BUILD_CONSOLE_APP)
  include(DownloadOpenSSL)
  include(DownloadPJSIP)
  include(DownloadJSON)
  return()
endif()

include(ModuleInclude)

if(BUILD_EXTERNAL_MULTI_ARCH)
  set(DJINNI_CUSTOM_COMMAND ${CMAKE_SOURCE_DIR}/run_djinni.sh ${DJINNI_SOURCES})
else()

  # Cause error in build-face, because it needs to be checked every time it builds 
  set(SCRIPT_STRING 
    "message(FATAL_ERROR \"djinni sources out of date -- please run 'superglue external'\")"
  )

  # If you change the location of the file, you need to run 'superglue external' twice
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/script-from-string.cmake.in
    ${EXTERNAL_PROJECT_BINARY_DIR}/cmake/script-from-string.cmake
  )

  set(DJINNI_CUSTOM_COMMAND 
    ${CMAKE_COMMAND} -P ${EXTERNAL_PROJECT_BINARY_DIR}/cmake/script-from-string.cmake
  )
endif()

ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_SOURCE_DIR}/output/all.djinni
  COMMAND ${DJINNI_CUSTOM_COMMAND}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  DEPENDS ${DJINNI_SOURCES_ABS}
)

ADD_CUSTOM_TARGET(Djinni ALL
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/output/all.djinni
  SOURCES ${DJINNI_SOURCES_ABS}
)

list(APPEND TARGET_DEPENDENCIES Djinni)

foreach(TARGET_LIB ${INSTALL_LIBS})
  get_property(TARGET_LIB_FILE TARGET ${TARGET_LIB} PROPERTY LOCATION)
  list(APPEND TARGET_LIB_FILES ${TARGET_LIB_FILE})
endforeach()

install(FILES ${TARGET_LIB_FILES} DESTINATION ${INSTALL_DESTINATION})

if(BUILD_EXTERNAL_MULTI_ARCH AND ${TARGET_PLATFORM} STREQUAL ANDROID)
  foreach(JAVA_SOURCE_DIR ${MODULE_DIRS})
    get_filename_component(DIR_NAME ${JAVA_SOURCE_DIR} NAME)
    install(DIRECTORY ${JAVA_SOURCE_DIR} DESTINATION "."
            PATTERN "*.djinni" EXCLUDE
            PATTERN "${DIR_NAME}/build"  EXCLUDE
            PATTERN "${DIR_NAME}/native" EXCLUDE
            PATTERN "${DIR_NAME}/xcodeproj" EXCLUDE
            PATTERN "*.iml" EXCLUDE
            )
  endforeach()
endif()

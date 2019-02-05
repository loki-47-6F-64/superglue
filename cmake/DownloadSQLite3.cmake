if(SQLITE_CMAKE)
  return()
endif()

set(SQLITE_CMAKE 1)


file(GLOB_RECURSE WRAPPER_SOURCES_SQLITE "${CMAKE_CURRENT_SOURCE_DIR}/wrappers/sqlite3/*.{cpp,hpp}")

list(APPEND WRAPPER_SOURCES ${WRAPPER_SOURCES_SQLITE})

if(${TARGET_PLATFORM} STREQUAL ANDROID)
  set(SQLITE_DIR_NAME ${EXTERNAL_PROJECT_BINARY_DIR}/sqlite-amalgamation-3080900)

  if(BUILD_EXTERNAL_MULTI_ARCH)
    file_download("http://www.sqlite.com/2015/sqlite-amalgamation-3080900.zip" "${SQLITE_DIR_NAME}.zip")

    message("Extracting [${SQLITE_DIR_NAME}.zip] to [${SQLITE_DIR_NAME}]")
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E tar xf "${SQLITE_DIR_NAME}.zip"
    )
  else()
    add_library(download-sqlite3 STATIC ${SQLITE_DIR_NAME}/sqlite3.c)
    set(SQLITE3_INCLUDE_DIRS ${SQLITE_DIR_NAME})
    set(SQLITE3_LIBRARIES download-sqlite3)
  endif()

elseif(${TARGET_PLATFORM} STREQUAL IOS)
  if(BUILD_EXTERNAL_MULTI_ARCH)
    message(WARNING "IOS already has sqlite3 through sqlite3.dylib")
  endif()
else()
  message(FATAL_ERROR "Unknown platform ${TARGET_PLATFORM}")
endif()

list(APPEND SQLITE3_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/wrappers/sqlite3)

list(APPEND HEADER_INCLUDE_DIRS ${SQLITE3_INCLUDE_DIRS})
list(APPEND TARGET_LIBRARIES ${SQLITE3_LIBRARIES})

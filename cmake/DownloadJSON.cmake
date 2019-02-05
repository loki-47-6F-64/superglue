if(NLOHMANN_JSON_CMAKE)
  return()
endif()

set(NLOHMANN_JSON_CMAKE 1)

set(binary_dir ${EXTERNAL_PROJECT_BINARY_DIR}/nlohmann)
# Some build-scripts build only a single architecture at a time.
if(BUILD_EXTERNAL_MULTI_ARCH)
  file_download("https://raw.githubusercontent.com/nlohmann/json/master/single_include/nlohmann/json.hpp" ${binary_dir}/nlohmann/json.hpp)
endif()

set(NLOHMANN_JSON_INCLUDE_DIRS "${binary_dir}")

list(APPEND HEADER_INCLUDE_DIRS ${NLOHMANN_JSON_INCLUDE_DIRS})

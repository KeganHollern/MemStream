cmake_minimum_required(VERSION 3.24)
project(MemStream)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# configure output directories
# Set output directories for all build types (Release, Debug, etc.)
set(BUILD_OUTPUTS ${CMAKE_CURRENT_SOURCE_DIR}/build/${CMAKE_SYSTEM_PROCESSOR})

message("Output directory: ${CMAKE_CURRENT_SOURCE_DIR}/build/${CMAKE_SYSTEM_PROCESSOR}")

# global compiler options
add_compile_options(-Wno-writable-strings -Wno-write-strings)

# options
option(ZYDIS_BUILD_TOOLS "" OFF)
option(ZYDIS_BUILD_EXAMPLES "" OFF)

add_subdirectory(deps/zydis)
add_subdirectory(src)

# examples
add_subdirectory(example/basic)
add_subdirectory(example/input)


cmake_minimum_required(VERSION 3.24)
project(MemStream)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# global compiler options
add_compile_options(-Wno-writable-strings)
add_definitions(-DLINUX)

# options
option(ZYDIS_BUILD_TOOLS "" OFF)
option(ZYDIS_BUILD_EXAMPLES "" OFF)

add_subdirectory(deps/zydis)
add_subdirectory(src)

# examples
add_subdirectory(example/basic)
add_subdirectory(example/input)

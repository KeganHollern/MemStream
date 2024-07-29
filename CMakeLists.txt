cmake_minimum_required(VERSION 3.24)
project(MemStream)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# MemProcFS
link_directories(${CMAKE_SOURCE_DIR}/lib/vmm)
include_directories(${CMAKE_SOURCE_DIR}/lib/vmm)

# MemStream headers
include_directories(${CMAKE_SOURCE_DIR}/include)

# compile memstream
file(GLOB src "${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.c*")
add_library(memstream SHARED ${src})
target_compile_definitions(memstream PRIVATE -DMEMSTREAM_EXPORTS)

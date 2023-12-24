cmake_minimum_required(VERSION 3.24)
project(MemStream)

set(CMAKE_CXX_STANDARD 17)

# Register Zydis dependency.
# Disable build of tools and examples.
option(ZYDIS_BUILD_TOOLS "" OFF)
option(ZYDIS_BUILD_EXAMPLES "" OFF)
add_subdirectory("deps/zydis")

add_library(MemStream FPGA.cpp Process.cpp Driver.cpp)
# add options to hide warnings for unsafe string type conversions
target_compile_options(MemStream PRIVATE -Wno-writable-strings)

# TODO: remove this when actually building for windows
add_definitions(-DLINUX)

include_directories(includes)

# TODO: fix linker errors
#find_library(LeechCore leechcore lib)
#target_link_libraries(MemStream LINK_PUBLIC ${LeechCore})

#find_library(MemProcFS vmm lib)
#target_link_libraries(MemStream LINK_PUBLIC ${MemProcFS})


target_link_libraries(MemStream PRIVATE "Zydis")
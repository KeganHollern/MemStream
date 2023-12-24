cmake_minimum_required(VERSION 3.23)
project(MemStream C)

set(CMAKE_C_STANDARD 17)

# Register Zydis dependency.
# Disable build of tools and examples.
option(ZYDIS_BUILD_TOOLS "" OFF)
option(ZYDIS_BUILD_EXAMPLES "" OFF)
add_subdirectory("deps/zydis")

add_library(MemStream FPGA.cpp FPGA.h)


include_directories(includes)

#find_library(LeechCore leechcore lib)
#target_link_libraries(MemStream LINK_PUBLIC ${LeechCore})

#ind_library(MemProcFS vmm lib)
#target_link_libraries(MemStream LINK_PUBLIC ${MemProcFS})

# link zydis
target_link_libraries(MemStream PRIVATE "Zydis")
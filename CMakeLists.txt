cmake_minimum_required(VERSION 3.24)
project(MemStream)

set(CMAKE_CXX_STANDARD 17)

# srcs and headers
include_directories(include)
file(GLOB SOURCES "src/*.cpp")
add_library(MemStream ${SOURCES})

# add options to hide warnings for unsafe string type conversions
target_compile_options(MemStream PRIVATE -Wno-writable-strings)

# TODO: tweak this for linux/windows builds...
add_definitions(-DLINUX -DMEMSTREAM_EXPORTS)

# reference memprocfs headers and libraries
include_directories(deps/memprocfs/include)
find_library(LeechCore leechcore ./deps/memprocfs/lib)
find_library(MemProcFS vmm ./deps/memprocfs/lib)
target_link_libraries(MemStream LINK_PUBLIC ${LeechCore} ${MemProcFS})

# Register Zydis dependency.
# Disable build of tools and examples.
option(ZYDIS_BUILD_TOOLS "" OFF)
option(ZYDIS_BUILD_EXAMPLES "" OFF)
add_subdirectory(deps/zydis)
target_link_libraries(MemStream PRIVATE Zydis)
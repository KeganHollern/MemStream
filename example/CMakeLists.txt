cmake_minimum_required(VERSION 3.24)
project(MemStreamExample)

set(CMAKE_CXX_STANDARD 17)

add_executable(MemStreamExample main.cpp)

# add options to hide warnings for unsafe string type conversions
target_compile_options(MemStream PRIVATE -Wno-writable-strings)

include_directories(../include)
find_library(MemStream MemStream ../cmake-build-debug)
target_link_libraries(MemStreamExample LINK_PUBLIC ${MemStream})

# TODO: tweak this for linux/windows builds...
add_definitions(-DLINUX)

# reference memprocfs headers and libraries
include_directories(../deps/memprocfs/include)
# TODO: try this on linux / windows and see if it links right...
#find_library(LeechCore leechcore ../deps/memprocfs/lib)
#find_library(MemProcFS vmm ../deps/memprocfs/lib)
#target_link_libraries(MemStreamExample LINK_PUBLIC ${LeechCore} ${MemProcFS})
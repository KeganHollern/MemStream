cmake_minimum_required(VERSION 3.24)
project(CfgDumpExample)

set(CMAKE_CXX_STANDARD 17)

add_executable(CfgDumpExample main.cpp)

# MemStream & MemProcFS links
target_link_libraries(CfgDumpExample memstream ${LeechCore} ${MemProcFS})

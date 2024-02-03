cmake_minimum_required(VERSION 3.24)
project(DetectedExample)

set(CMAKE_CXX_STANDARD 17)

add_executable(DetectedExample main.cpp)

# MemStream & MemProcFS links
target_link_libraries(DetectedExample memstream ${LeechCore} ${MemProcFS})

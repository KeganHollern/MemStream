cmake_minimum_required(VERSION 3.24)
project(BasicExample)

set(CMAKE_CXX_STANDARD 17)

add_executable(BasicExample main.cpp)

# MemStream & MemProcFS links
target_link_libraries(BasicExample memstream LeechCore MemProcFS)

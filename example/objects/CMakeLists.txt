cmake_minimum_required(VERSION 3.24)
project(ObjectExample)

set(CMAKE_CXX_STANDARD 17)

add_executable(ObjectExample main.cpp)

# MemStream & MemProcFS links
target_link_libraries(ObjectExample memstream LeechCore MemProcFS)
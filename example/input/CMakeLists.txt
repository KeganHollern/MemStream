cmake_minimum_required(VERSION 3.24)
project(InputExample)

set(CMAKE_CXX_STANDARD 17)

add_executable(InputExample main.cpp)

# MemStream & MemProcFS links
target_link_libraries(InputExample PRIVATE MemStream)
target_link_libraries(InputExample PRIVATE ${LeechCore} ${MemProcFS})
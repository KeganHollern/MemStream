cmake_minimum_required(VERSION 3.24)
project(MonoExample)

set(CMAKE_CXX_STANDARD 17)

add_executable(MonoExample main.cpp)

# MemStream & MemProcFS links
target_compile_options(MonoExample PRIVATE "-utf-8")
target_link_options(MonoExample PRIVATE "-v")
target_link_libraries(MonoExample memstream ${LeechCore} ${MemProcFS})
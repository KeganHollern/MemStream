cmake_minimum_required(VERSION 3.24)
project(MemStream_ReClassPlugin)

set(CMAKE_CXX_STANDARD 17)

add_library(MemStream_ReClassPlugin SHARED main.cpp metadata.rc)

# MemStream & MemProcFS links
target_compile_options(MemStream_ReClassPlugin PRIVATE "-utf-8")
target_link_options(MemStream_ReClassPlugin PRIVATE "-v")
target_link_libraries(MemStream_ReClassPlugin memstream ${LeechCore} ${MemProcFS})
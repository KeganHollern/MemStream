cmake_minimum_required(VERSION 3.23)
project(MemStream C)

set(CMAKE_C_STANDARD 17)

add_library(MemStream
        pcie.c
        proc.c
        util.c
        read.c
        hashmap.c
        )


include_directories(includes)

find_library(LeechCore leechcore lib)
target_link_libraries(MemStream LINK_PUBLIC ${LeechCore})

find_library(MemProcFS vmm lib)
target_link_libraries(MemStream LINK_PUBLIC ${MemProcFS})
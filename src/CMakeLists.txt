file(GLOB SOURCES "./*.cpp")
add_library(MemStream SHARED ${SOURCES})

target_include_directories(MemStream PUBLIC ../include)

# MemProcFS Headers
target_include_directories(MemStream PUBLIC ../deps/memprocfs/include)

# MemProcFS Libraries
find_library(LeechCore leechcore ../deps/memprocfs/lib)
find_library(MemProcFS vmm ../deps/memprocfs/lib)

# Link to Deps
target_link_libraries(MemStream PRIVATE Zydis)
target_link_libraries(MemStream PRIVATE ${LeechCore} ${MemProcFS})
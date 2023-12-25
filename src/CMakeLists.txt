file(GLOB ROOT "./*.cpp")
file(GLOB WIN "./win/*.cpp")
add_library(MemStream SHARED ${ROOT} ${WIN})

target_include_directories(MemStream PUBLIC ../include)

# MemProcFS Headers
target_include_directories(MemStream PUBLIC ../deps/memprocfs/include)

# MemProcFS Libraries
find_library(LeechCore leechcore ../deps/memprocfs/lib)
find_library(MemProcFS vmm ../deps/memprocfs/lib)

# Link to Deps
target_link_libraries(MemStream PRIVATE Zydis)
target_link_libraries(MemStream PRIVATE ${LeechCore} ${MemProcFS})
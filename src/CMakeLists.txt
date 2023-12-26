file(GLOB ROOT "./*.cpp")
file(GLOB WIN "./win/*.cpp")
add_library(MemStream SHARED ${ROOT} ${WIN})

set_target_properties(MemStream PROPERTIES PREFIX "")

target_include_directories(MemStream PUBLIC ${CMAKE_SOURCE_DIR}/include)

# MemProcFS Headers
target_include_directories(MemStream PUBLIC ${CMAKE_SOURCE_DIR}/deps/memprocfs/include)

# MemProcFS Libraries
find_library(LeechCore leechcore ${CMAKE_SOURCE_DIR}/deps/memprocfs/lib/${CMAKE_SYSTEM_PROCESSOR} REQUIRED)
find_library(MemProcFS vmm ${CMAKE_SOURCE_DIR}/deps/memprocfs/lib/${CMAKE_SYSTEM_PROCESSOR} REQUIRED)

Message("Linking to Leechcore: ${CMAKE_SOURCE_DIR}/deps/memprocfs/lib/${CMAKE_SYSTEM_PROCESSOR}/leechcore.so")
Message("Linking to MemProcFS: ${CMAKE_SOURCE_DIR}/deps/memprocfs/lib/${CMAKE_SYSTEM_PROCESSOR}/vmm.so")

# Link to Deps
target_link_libraries(MemStream Zydis)
target_link_libraries(MemStream ${LeechCore} ${MemProcFS})
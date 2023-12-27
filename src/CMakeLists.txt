file(GLOB ROOT "./*.cpp")
file(GLOB WIN "./win/*.cpp")
add_library(memstream SHARED ${ROOT} ${WIN})

set_target_properties(memstream PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${BUILD_OUTPUTS}
        LIBRARY_OUTPUT_DIRECTORY ${BUILD_OUTPUTS}
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_OUTPUTS})

target_include_directories(memstream PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)

# MemProcFS Headers
target_include_directories(memstream PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../deps/memprocfs/include)

# MemProcFS Libraries
set_target_properties(memstream PROPERTIES PREFIX "")

add_library(LeechCore SHARED IMPORTED)
set_target_properties(LeechCore PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../deps/memprocfs/lib/${CMAKE_SYSTEM_PROCESSOR}/leechcore.so)

add_library(MemProcFS SHARED IMPORTED)
set_target_properties(MemProcFS PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../deps/memprocfs/lib/${CMAKE_SYSTEM_PROCESSOR}/vmm.so)

Message("Linking to Leechcore: ${CMAKE_CURRENT_SOURCE_DIR}/../deps/memprocfs/lib/${CMAKE_SYSTEM_PROCESSOR}/leechcore.so")
Message("Linking to MemProcFS: ${CMAKE_CURRENT_SOURCE_DIR}/../deps/memprocfs/lib/${CMAKE_SYSTEM_PROCESSOR}/vmm.so")

# Link to Deps
target_link_libraries(memstream Zydis LeechCore MemProcFS)
target_link_options(memstream PRIVATE "-v")
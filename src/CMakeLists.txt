file(GLOB ROOT "./*.cpp")
file(GLOB WIN "./win/*.cpp")
file(GLOB DMA "./dma/*.cpp")
add_library(memstream SHARED ${ROOT} ${WIN} ${DMA})

set_target_properties(memstream PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${BUILD_OUTPUTS}
        LIBRARY_OUTPUT_DIRECTORY ${BUILD_OUTPUTS}
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_OUTPUTS})

target_compile_definitions(memstream PRIVATE -DMEMSTREAM_EXPORTS)

target_include_directories(memstream PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)

# MemProcFS Headers
target_include_directories(memstream PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../deps/memprocfs/include)

# MemProcFS Libraries
set_target_properties(memstream PROPERTIES PREFIX "")

Message("Linking to Leechcore: ${LeechCore}")
Message("Linking to MemProcFS: ${MemProcFS}")

# Link to Deps
target_link_libraries(memstream PUBLIC Zydis ${LeechCore} ${MemProcFS})
target_link_options(memstream PRIVATE "-v")
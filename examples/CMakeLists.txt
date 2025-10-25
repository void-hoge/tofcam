add_executable(capture capture.cpp)
target_link_libraries(capture
    PRIVATE tofcam
)

add_executable(loadtest loadtest.cpp)
target_link_libraries(loadtest
    PRIVATE tofcam
)

add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark
    PRIVATE tofcam
)

add_executable(neon_capture neon_capture.cpp)
target_link_libraries(neon_capture
    PRIVATE tofcam
)

add_executable(neon_loadtest neon_loadtest.cpp)
target_link_libraries(neon_loadtest
    PRIVATE tofcam
)

add_executable(neon_benchmark neon_benchmark.cpp)
target_link_libraries(neon_benchmark
    PRIVATE tofcam
)

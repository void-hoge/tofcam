add_executable(capture main.cpp)
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

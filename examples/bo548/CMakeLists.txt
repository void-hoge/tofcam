add_executable(capture_bo548 capture.cpp)
target_link_libraries(capture_bo548
    PRIVATE tofcam
)

add_executable(captureraw_bo548 captureraw.cpp)
target_link_libraries(captureraw_bo548
    PRIVATE tofcam
)

add_executable(loadtest_bo548 loadtest.cpp)
target_link_libraries(loadtest_bo548
    PRIVATE tofcam
)

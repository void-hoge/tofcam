add_executable(capture capture.cpp)
target_link_libraries(capture
    PRIVATE tofcam
)

add_executable(captureraw captureraw.cpp)
target_link_libraries(captureraw
    PRIVATE tofcam
)

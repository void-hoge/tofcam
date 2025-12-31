add_executable(capture capture.cpp)
target_link_libraries(capture
    PRIVATE tofcam
)

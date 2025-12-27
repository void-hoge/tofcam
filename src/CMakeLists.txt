add_library(tofcam
    camera.cpp
    syscall.cpp
    utility.cpp
    fakecam.cpp
    buffpool.cpp
)

target_include_directories(tofcam
    PUBLIC ../include
)

target_compile_features(tofcam
    PUBLIC cxx_std_20
)

target_compile_options(tofcam
    PRIVATE -Wall -Wextra -O3 -mtune=native -march=native -ffast-math -fno-math-errno -funroll-loops
)

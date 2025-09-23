add_library(tofcam
    tofcam.cpp
    syscall.cpp
    utility.cpp
)

target_include_directories(tofcam
    PUBLIC ../include
)

target_compile_features(tofcam
    PUBLIC cxx_std_17
)

target_compile_options(tofcam
    PRIVATE -Wall -Wextra -O3 -mtune=native -march=native
)

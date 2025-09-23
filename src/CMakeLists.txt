add_library(tofcam
        tofcam.cpp
        syscall.cpp
)

target_include_directories(tofcam
        PUBLIC ../include
)

target_compile_features(tofcam
        PUBLIC cxx_std_17
)

target_compile_options(tofcam
        PRIVATE -Wall -Wextra
)

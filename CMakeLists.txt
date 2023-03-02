cmake_minimum_required(VERSION 3.20)
project(gis)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-fsanitize=address")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

include_directories(
    ./include
    /usr/include/opencv4
)

link_libraries(
    -lopencv_calib3d
    -lopencv_imgproc
    # -lopencv_contrib
    # -lopencv_legacy
    -lopencv_imgcodecs
    -lopencv_core
    -lopencv_ml
    -lopencv_features2d
    -lopencv_objdetect
    -lopencv_flann
    -lopencv_video
    -lopencv_highgui
)

add_executable(
    lidar

    include/defines.hpp
    include/utils.hpp

    src/main.cpp
    src/utils.cpp
)

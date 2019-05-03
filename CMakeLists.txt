cmake_minimum_required(VERSION 3.14)
project(godot_webcam)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake-modules)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(godot-cpp)

add_library(${PROJECT_NAME} SHARED src/init.cpp src/init.cpp src/webcam.cpp src/webcam.h src/webcamV4l2.cpp src/webcamV4l2.h src/jpgd.h src/jpgd.cpp)

find_package(V4L2)
if (V4L2_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${V4L2_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${V4L2_LIBRARIES})
    add_definitions(-DV4L2_FOUND)
else ()
    message("No webcam library found --> Currently only Video4Linux2 is supported")
endif ()

target_link_libraries(${PROJECT_NAME} godot-cpp)


# copy generated file to godot test project
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_BINARY_DIR}/libgodot_webcam.so"
        "${PROJECT_SOURCE_DIR}/webcam-sample-project/webcam/libgodot_webcam.so")

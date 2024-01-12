cmake_minimum_required(VERSION 3.10)

project(visualizer)
set(CMAKE_C_STANDARD 99)

add_subdirectory(libs/raylib)
add_executable(visualizer main.c)

target_link_libraries(visualizer PRIVATE raylib m)

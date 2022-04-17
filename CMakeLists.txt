cmake_minimum_required(VERSION 3.21)
project(ch8emu)

set(CMAKE_CXX_STANDARD 20)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(ch8emu main.cpp Chip8.cpp Chip8.h)

target_link_libraries(ch8emu ${SDL2_LIBRARIES})

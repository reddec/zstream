cmake_minimum_required(VERSION 2.8)
project(zstream C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Wall -Wextra -Wpedantic")
find_library(ZMQ zmq REQUIRED)
set(SOURCE_FILES main.c)
add_executable(zstream ${SOURCE_FILES})
target_link_libraries(zstream ${ZMQ})
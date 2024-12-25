cmake_minimum_required(VERSION 3.9)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

project(RobustTimer VERSION 0.0.0 DESCRIPTION "Robust and re-usable C++ wrapper for posix timers")
add_library(RobustTimer STATIC source/RobustTimer.cpp)

target_include_directories(RobustTimer PRIVATE include)
target_include_directories(RobustTimer PRIVATE src)
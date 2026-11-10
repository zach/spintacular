# The testbed's CMake project, written by automap start.
# SPDX-License-Identifier: MIT-0
cmake_minimum_required(VERSION 3.25)
project(testbed LANGUAGES C CXX)

include(cmake/automap/automap.cmake)

add_executable(testbed main.cpp)
target_link_libraries(testbed PRIVATE automap_glue)

cmake_minimum_required(VERSION 3.7)
project(kepler)

message(STATUS "*** CMakeLists for Kepler")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_VERBOSE_MAKEFILE ON)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_subdirectory(bluzelled) # The daemon/console application

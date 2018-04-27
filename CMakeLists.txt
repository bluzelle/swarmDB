cmake_minimum_required(VERSION 3.10)

project(bluzelle CXX)

# use ccache if available...
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

# output dir
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)

# compiler options...
set(CMAKE_CXX_STANDARD 17)
add_compile_options("-fdiagnostics-color=auto")

set(warnings "-Wno-deprecated-declarations -Wall -Wextra -Werror -Wpedantic")
set(CMAKE_CXX_FLAGS ${warnings})
set(CMAKE_C_FLAGS   ${warnings})

# find packages
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(cmake/add_gmock_test.cmake)

set(REQUIRED_BOOST "1.67.0")
set(Boost_USE_STATIC_LIBS on)

find_package(
        Boost ${REQUIRED_BOOST}
        REQUIRED COMPONENTS
        log
        program_options
        system
        thread
        serialization
)

if(Boost_FOUND)
    message(STATUS "Boost: ${Boost_INCLUDE_DIRS}")
    include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
endif()

find_package(PkgConfig REQUIRED)
pkg_search_module(JSONCPP REQUIRED jsoncpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# googletest (todo: we may want to have a flag enabling/disabling tests?)
include(cmake/googletest.cmake)
enable_testing()

add_subdirectory(bootstrap)
add_subdirectory(crud)
add_subdirectory(node)
add_subdirectory(options)
add_subdirectory(raft)
add_subdirectory(storage)
add_subdirectory(swarm)
add_subdirectory(pkg)


cmake_minimum_required(VERSION 3.10)

project(bluzelle CXX)

include(cmake/swarm_version.cmake)

message("Build version ${PROJECT_VERSION}")

# Setup version header
configure_file(${PROJECT_SOURCE_DIR}/swarm_version.hpp.in ${CMAKE_BINARY_DIR}/swarm_version.hpp.tmp)
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/swarm_version.hpp.tmp ${CMAKE_BINARY_DIR}/swarm_version.hpp)
execute_process(COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/swarm_version.hpp.tmp)
include_directories(${CMAKE_BINARY_DIR})

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

set(warnings "-Wno-deprecated-declarations -Wall -Wextra -Werror -Wpedantic -Wno-extended-offsetof")
set(CMAKE_CXX_FLAGS ${warnings})
set(CMAKE_C_FLAGS   ${warnings})

# find packages
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
add_subdirectory(ethereum)
add_subdirectory(http)
add_subdirectory(node)
add_subdirectory(options)
add_subdirectory(pkg)
add_subdirectory(proto)
add_subdirectory(raft)
add_subdirectory(storage)
add_subdirectory(swarm)
add_subdirectory(utils)

cmake_minimum_required(VERSION 3.10)

project(bluzelle CXX)

include(cmake/swarm_version.cmake)

message("Build version ${PROJECT_VERSION}")

# Setup version header
configure_file(${PROJECT_SOURCE_DIR}/cmake/swarm_version.hpp.in ${CMAKE_BINARY_DIR}/swarm_version.hpp.tmp)
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

if (APPLE)
    add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)
endif(APPLE)

set(warnings "-Wno-deprecated-declarations -Wall -Wextra -Werror -Wpedantic")
if (APPLE)
    set(warnings "${warnings} -Wno-extended-offsetof")
endif(APPLE)

set(CMAKE_CXX_FLAGS ${warnings})
set(CMAKE_C_FLAGS   ${warnings})

# find packages
include(cmake/add_gmock_test.cmake)

include(FindProtobuf)
find_package(Protobuf REQUIRED)
include_directories(SYSTEM ${PROTOBUF_INCLUDE_DIR})

set(REQUIRED_BOOST "1.68.0")
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

find_package(CURL REQUIRED)

message("OpenSSL version: ${OPENSSL_VERSION}")


include_directories(${CMAKE_CURRENT_SOURCE_DIR})

include(cmake/jsoncpp.cmake)
include(cmake/rocksdb.cmake)
include(cmake/googletest.cmake)
include(cmake/openssl.cmake)

include_directories(SYSTEM ${OPENSSL_INCLUDE_DIR})

enable_testing()

# coverage?
if (ENABLE_GCOV)
    if (NOT APPLE)
        set(COVERAGE_COMPILER_FLAGS "-g -O0 --coverage -fprofile-arcs -ftest-coverage" CACHE INTERNAL "")
        set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   ${COVERAGE_COMPILER_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COVERAGE_COMPILER_FLAGS}")
        message(STATUS "Appending code coverage compiler flags: ${COVERAGE_COMPILER_FLAGS}")
    endif()
endif()

add_subdirectory(bootstrap)
add_subdirectory(crud)
add_subdirectory(ethereum)
add_subdirectory(http)
add_subdirectory(node)
add_subdirectory(options)
add_subdirectory(pkg)
add_subdirectory(proto)
add_subdirectory(raft)
add_subdirectory(status)
add_subdirectory(storage)
add_subdirectory(swarm)
add_subdirectory(utils)
add_subdirectory(audit)
add_subdirectory(pbft)
add_subdirectory(chaos)
add_subdirectory(crypto)

include(cmake/static_analysis.cmake)

# git commit used to build
add_custom_command(
    OUTPUT  swarm_git_commit
    COMMENT "Generating swarm_git_commit.hpp"
    COMMAND ${CMAKE_COMMAND}
    -D PROJECT_SOURCE_DIR=${CMAKE_SOURCE_DIR}
    -D PROJECT_BINARY_DIR=${CMAKE_BINARY_DIR}
    -P ${CMAKE_SOURCE_DIR}/cmake/git_commit.cmake
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(swarm-git-commit ALL DEPENDS swarm_git_commit)

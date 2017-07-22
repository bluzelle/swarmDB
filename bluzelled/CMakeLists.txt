cmake_minimum_required(VERSION 3.7)
project(bluzelled)

message(STATUS "*** CMakeLists for Bluzelled")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_compile_options(-Wnested-anon-types)

find_package(Boost 1.64.0 REQUIRED COMPONENTS system coroutine thread REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

include_directories(
        inc
        inc/containers
)

set(SOURCE_FILES
        inc/Node.h
        inc/containers/CMap.h
        inc/containers/CSet.h
        src/main.cpp
        src/Node.cpp
        src/Task.cpp
        inc/Task.h
        inc/NodeUtilities.h
        src/NodeUtilities.cpp
        )

add_executable(bluzelled ${SOURCE_FILES})
target_link_libraries(bluzelled ${Boost_LIBRARIES})


################################################################
# Unit tests
find_package(Boost 1.64.0 COMPONENTS unit_test_framework thread system timer REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

enable_testing()
set(SOURCE_FILES
        inc/containers/CMap.h
        src/test.cpp
        inc/containers/CSet.h
        src/Task.cpp
        inc/Task.h
        inc/NodeUtilities.h
        src/NodeUtilities.cpp)



add_compile_options(-Wnested-anon-types)

file(COPY assets/words.txt DESTINATION ${CMAKE_BINARY_DIR})
add_executable(bluzelled_test ${SOURCE_FILES})
target_link_libraries(bluzelled_test ${Boost_LIBRARIES})



message(STATUS "\n\n")
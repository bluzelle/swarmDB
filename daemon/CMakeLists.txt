cmake_minimum_required(VERSION 3.9)

set(CMAKE_CXX_STANDARD 17)
set(REQUIRED_BOOST "1.65.0")

if(MSVC)
    message(STATUS "*** building make for Windows")
    add_compile_options(/W4)
elseif(UNIX AND NOT APPLE)
    message(STATUS "*** building make for Unix")
    add_compile_options(-lrt)
elseif(APPLE)
    message(STATUS "*** building make for MacOS")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# TODO can we move this to the root CMakeLists.txt?
find_package(Boost ${REQUIRED_BOOST} REQUIRED COMPONENTS unit_test_framework system coroutine thread regex chrono random REQUIRED)
if(Boost_FOUND)
    message(STATUS "Your boost is here: ${Boost_INCLUDE_DIRS}")
    check_asio_version()
    check_beast_version()
    include_directories(${Boost_INCLUDE_DIRS})
endif()

###############################################################################
# Primary project for the_db
project(the_db)
set(SOURCE_FILES main.cpp)
add_executable(the_db ${SOURCE_FILES})
target_link_libraries(the_db ${Boost_LIBRARIES})

###############################################################################
# Unit tests
message(STATUS "*** CMakeLists for bluzelled_test")

set(SOURCE_FILES test.cpp)
add_executable(the_db_test ${SOURCE_FILES})
target_link_libraries(the_db_test ${Boost_LIBRARIES})

cmake_minimum_required(VERSION 3.9)

function(check_asio_version)
    set(ASIO_VERSION_HPP "${Boost_INCLUDE_DIRS}/boost/asio/version.hpp")
    set(REQUIRED_ASIO "101010")
    IF(EXISTS ${ASIO_VERSION_HPP})
        FILE(READ ${ASIO_VERSION_HPP} contents)
        STRING(REGEX MATCH "#define BOOST_ASIO_VERSION [0-9]*" contents ${contents})
        STRING(COMPARE EQUAL "#define BOOST_ASIO_VERSION ${REQUIRED_ASIO}" ${contents} _cmp)
        IF(NOT _cmp)
            message(FATAL_ERROR "\n!!!\nYour BOOST does not contain the correct version of ASIO;\n\t${contents}\nthe correct version is ${REQUIRED_ASIO}.")
        ENDIF()
    ELSE()
        message(FATAL_ERROR "\n!!!\nYour BOOST does not contain asio(which is odd)...")
    ENDIF()
endfunction()

function(check_boost_version)
    set(BEAST_VERSION_HPP "${Boost_INCLUDE_DIRS}/boost/beast/version.hpp")
    set(REQUIRED_BEAST "123")
    IF(EXISTS ${BEAST_VERSION_HPP})
        FILE(READ ${BEAST_VERSION_HPP} contents)
        STRING(REGEX  MATCH "#define BOOST_BEAST_VERSION [0-9]*" contents ${contents})
        STRING(COMPARE EQUAL "#define BOOST_BEAST_VERSION ${REQUIRED_BEAST}" ${contents} _cmp )
        if(NOT _cmp)
            message(FATAL_ERROR "\n!!!\nYour BOOST does not contain the correct version of BEAST;\n\t${contents}\nthe correct version is ${REQUIRED_BEAST}.")
        endif()
    ELSE()
        message(FATAL_ERROR "\n!!!\nYour BOOST does not contain the beast library. Grab it from the boost github.\n\n")
    ENDIF()
endfunction()


project(the_db)

set(CMAKE_CXX_STANDARD 17)
set(REQUIRED_BOOST "1.65.0")


if(MSVC)
    message(STATUS "*** compiling for Windows")
    add_compile_options(/W4)
elseif(UNIX AND NOT APPLE)
    message(STATUS "*** compiling for Unix")
    add_compile_options(-lrt)
elseif(APPLE)
    message(STATUS "*** compiling for MacOS")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# TODO can we move this to the root CMakeLists.txt?
find_package(Boost ${REQUIRED_BOOST} REQUIRED COMPONENTS unit_test_framework system coroutine thread regex chrono random REQUIRED)
if(Boost_FOUND)
    check_asio_version()
    check_boost_version()
    include_directories(${Boost_INCLUDE_DIRS})
endif()


set(SOURCE_FILES main.cpp)
add_executable(the_db ${SOURCE_FILES})

###############################################################################
# Unit tests
message(STATUS "*** CMakeLists for bluzelled_test")

set(SOURCE_FILES test.cpp)
add_executable(the_db_test ${SOURCE_FILES})
target_link_libraries(the_db_test ${Boost_LIBRARIES})

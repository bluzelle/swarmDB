cmake_minimum_required(VERSION 3.9)

function(list_cmake_properties)
    get_cmake_property(_variableNames VARIABLES)
    foreach (_variableName ${_variableNames})
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
endfunction()

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

###############################################################################
##
project(bluzelled)

###############################################################################
## bluzelled
message(STATUS "*** CMakeLists for Bluzelled Starts")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(REQUIRED_BOOST "1.65.0")

file(COPY assets/web DESTINATION ${CMAKE_BINARY_DIR})

if(MSVC)
    add_compile_options(/W4)
elseif(UNIX AND NOT APPLE)
    message(STATUS "***** UNIX!!!!!!!!!!!!!!!")
    add_compile_options(-lrt)
elseif(APPLE)
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()


find_package(Boost ${REQUIRED_BOOST} REQUIRED COMPONENTS system coroutine thread regex chrono random REQUIRED)
if(Boost_FOUND)
    check_asio_version()
    check_boost_version()
    include_directories(${Boost_INCLUDE_DIRS})
endif()

include_directories(
        inc
        inc/containers
        inc/http
)

set(INCLUDE_FILES
        inc/containers/CMap.h
        inc/containers/CSet.h
        inc/ethereum/EthereumApi.h
        inc/ethereum/EthereumToken.h
        inc/http/server/connection.hpp
        inc/http/server/connection_manager.hpp
        inc/http/server/header.hpp
        inc/http/server/mime_types.hpp
        inc/http/server/reply.hpp
        inc/http/server/request.hpp
        inc/http/server/request_handler.hpp
        inc/http/server/request_parser.hpp
        inc/http/server/server.hpp
        inc/http/server/ServiceManager.h
        inc/Node.h
        inc/NodeManager.h
        inc/NodeUtilities.h
        inc/services/CountNodes.h
        inc/services/GetAllNodes.h
        inc/services/GetMaxNodes.h
        inc/services/GetMinNodes.h
        inc/services/Ping.h
        inc/services/Quit.h
        inc/services/RemoveNodes.h
        inc/services/Service.h
        inc/services/Services.h
        inc/services/SetMaxNodes.h
        inc/services/UpdateNodes.h
        inc/Task.h
        inc/web_sockets/Listener.h
        inc/web_sockets/Session.h
        inc/web_sockets/WebSocket.h
        inc/web_sockets/WebSocketServer.h
        )


set(SOURCE_FILES
        src/ethereum/EthereumApi.cpp
        src/http/server/connection.cpp
        src/http/server/connection_manager.cpp
        src/http/server/mime_types.cpp
        src/http/server/reply.cpp
        src/http/server/request_handler.cpp
        src/http/server/request_parser.cpp
        src/http/server/server.cpp
        src/main.cpp
        src/Node.cpp
        src/NodeManager.cpp
        src/NodeUtilities.cpp
        src/services/GetAllNodes.cpp
        src/services/Ping.cpp
        src/services/Service.cpp
        src/services/Services.cpp
        src/Task.cpp
        src/web_sockets/Listener.cpp
        src/web_sockets/Session.cpp
        )

add_executable(bluzelled ${SOURCE_FILES} ${INCLUDE_FILES})

target_link_libraries(bluzelled
        ${Boost_LIBRARIES}
        )
target_link_libraries(bluzelled
        ${Boost_CHRONO_LIBRARIES}
        )
target_link_libraries(bluzelled
        ${Boost_RANDOM_LIBRARIES}
        )
if(UNIX AND NOT APPLE)
target_link_libraries(bluzelled
        pthread
        )
endif()

###############################################################################
# Unit tests
message(STATUS "*** CMakeLists for bluzelled_test")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE OFF)
if(MSVC)
    message(STATUS "***** MSVC *****")
    add_compile_options(/W4)
elseif(UNIX AND NOT APPLE)
    message(STATUS "***** UNIX *****")
    add_compile_options(-lrt)
else(APPLE)
    message(STATUS "***** APPLE *****")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()


find_package(Boost ${REQUIRED_BOOST} COMPONENTS unit_test_framework thread system timer regex chrono random REQUIRED)
if(Boost_FOUND)
    check_asio_version()
    check_boost_version()
    include_directories(${Boost_INCLUDE_DIRS})
endif()

enable_testing()

set(INCLUDE_FILES
        inc/containers/CMap.h
        inc/containers/CSet.h
        inc/ethereum/EthereumApi.h
        inc/NodeManager.h
        inc/NodeUtilities.h
        inc/services/CountNodes.h
        inc/services/GetAllNodes.h
        inc/services/GetMaxNodes.h
        inc/services/Ping.h
        inc/services/Quit.h
        inc/services/Service.h
        inc/services/Services.h
        inc/services/SetMaxNodes.h
        inc/Task.h
        inc/web_sockets/Listener.h
        inc/web_sockets/Session.h
        inc/web_sockets/WebSocketServer.h
        )

set(SOURCE_FILES
        src/cmap_test.cpp
        src/ethereum/ethereum_test.cpp
        src/ethereum/EthereumApi.cpp
        src/Node.cpp
        src/node_test.cpp
        src/NodeManager.cpp
        src/NodeUtilities.cpp
        src/services/GetAllNodes.cpp
        src/services/GetAllNodes.cpp
        src/services/Ping.cpp
        src/services/Service.cpp
        src/services/service_tests.cpp
        src/services/Services.cpp
        src/Task.cpp
        src/test.cpp
        src/web_sockets/Listener.cpp
        src/web_sockets/Session.cpp
        src/web_sockets/web_socket_tests.cpp
        src/node_manager_tests.cpp
        )

file(COPY assets/words.txt DESTINATION ${CMAKE_BINARY_DIR})
add_executable(bluzelled_test ${SOURCE_FILES} ${INCLUDE_FILES})

target_link_libraries(bluzelled_test
        ${Boost_LIBRARIES}
        )
target_link_libraries(bluzelled_test
        ${Boost_CHRONO_LIBRARIES}
        )
target_link_libraries(bluzelled_test
        ${Boost_RANDOM_LIBRARIES}
        )
if(UNIX AND NOT APPLE)
target_link_libraries(bluzelled_test
        pthread
        )
endif()

message(STATUS "*** CMakeLists for Bluzelled Ends\n\n")

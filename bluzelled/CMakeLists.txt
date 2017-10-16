cmake_minimum_required(VERSION 3.9)

function(list_cmake_properties)
    get_cmake_property(_variableNames VARIABLES)
    foreach (_variableName ${_variableNames})
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
endfunction()

project(bluzelled)
###############################################################################
## bluzelled
message(STATUS "*** CMakeLists for Bluzelled Starts")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE OFF)

file(COPY assets/web DESTINATION ${CMAKE_BINARY_DIR})

if(MSVC)
    add_compile_options(/W4)
elseif(UNIX AND NOT APPLE)
    message(STATUS "***** UNIX!!!!!!!!!!!!!!!")
    add_compile_options(-lrt)
elseif(APPLE)
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()


find_package(Boost 1.65.0 REQUIRED COMPONENTS system coroutine thread regex chrono random REQUIRED)
if(Boost_FOUND)
    message(STATUS "**** ${Boost_INCLUDE_DIRS} ****")
    #if(NOT EXISTS "${ROOT}/configuration/${customer}/configuration.${project_name}.xml")
        #file(COPY ../third_party/boost/beast/include/boost/ DESTINATION ${Boost_INCLUDE_DIRS}/boost)
    #endif()
    include_directories(${Boost_INCLUDE_DIRS})
endif()

include_directories(
        inc
        inc/containers
        inc/http
)

set(SOURCE_FILES

        inc/containers/CMap.h
        inc/containers/CSet.h
        inc/Node.h
        inc/services/GetMinNodes.h
        inc/services/RemoveNodes.h
        inc/services/UpdateNodes.h
        src/main.cpp
        src/Node.cpp
        src/Task.cpp
        inc/Task.h
        inc/NodeUtilities.h
        src/NodeUtilities.cpp
        src/web_sockets/Listener.cpp
        inc/web_sockets/Listener.h
        src/web_sockets/Session.cpp
        inc/web_sockets/Session.h
        src/services/Services.cpp
        inc/services/Services.h
        src/services/Service.cpp
        inc/services/Service.h
        src/services/Ping.cpp
        inc/services/Ping.h
        src/services/GetAllNodes.cpp
        inc/services/GetAllNodes.h
        inc/web_sockets/WebSocket.h
        inc/services/CountNodes.h
        inc/services/SetMaxNodes.h
        src/NodeManager.cpp
        inc/NodeManager.h
        inc/services/Quit.h
        inc/services/GetMaxNodes.h
        inc/services/GetMinNodes.h
        inc/web_sockets/WebSocketServer.h
        inc/http/server/connection.hpp
        inc/http/server/connection_manager.hpp
        inc/http/server/header.hpp
        inc/http/server/mime_types.hpp
        inc/http/server/reply.hpp
        inc/http/server/request.hpp
        inc/http/server/request_handler.hpp
        inc/http/server/request_parser.hpp
        inc/http/server/server.hpp
        src/http/server/connection.cpp
        src/http/server/connection_manager.cpp
        src/http/server/mime_types.cpp
        src/http/server/reply.cpp
        src/http/server/request_handler.cpp
        src/http/server/request_parser.cpp
        src/http/server/server.cpp

        )

add_executable(bluzelled ${SOURCE_FILES})

target_link_libraries(bluzelled
        ${Boost_LIBRARIES}
        )
target_link_libraries(bluzelled
        ${Boost_CHRONO_LIBRARIES}
        )
target_link_libraries(bluzelled
        ${Boost_RANDOM_LIBRARIES}
        )
target_link_libraries(bluzelled
        pthread
        )


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


find_package(Boost 1.64.0 COMPONENTS unit_test_framework thread system timer regex chrono random REQUIRED)
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
        src/cmap_test.cpp
        src/node_test.cpp
        src/web_sockets/Listener.cpp
        inc/web_sockets/Listener.h
        src/web_sockets/Session.cpp
        inc/web_sockets/Session.h
        src/web_sockets/web_socket_tests.cpp
        src/services/Services.cpp
        inc/services/Services.h
        src/services/Service.cpp
        inc/services/Service.h
        src/services/Ping.cpp
        inc/services/Ping.h
        src/services/GetAllNodes.cpp
        inc/services/GetAllNodes.h
        src/services/service_tests.cpp
        inc/services/CountNodes.h
        inc/services/SetMaxNodes.h
        src/NodeManager.cpp
        inc/NodeManager.h
        inc/services/Quit.h
        inc/services/GetMaxNodes.h
        inc/web_sockets/WebSocketServer.h
        )

file(COPY assets/words.txt DESTINATION ${CMAKE_BINARY_DIR})
add_executable(bluzelled_test ${SOURCE_FILES})

target_link_libraries(bluzelled_test
        ${Boost_LIBRARIES}
        )
target_link_libraries(bluzelled_test
        ${Boost_CHRONO_LIBRARIES}
        )
target_link_libraries(bluzelled_test
        ${Boost_RANDOM_LIBRARIES}
        )

message(STATUS "*** CMakeLists for Bluzelled Ends\n\n")

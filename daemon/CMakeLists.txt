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

set(Boost_USE_STATIC_LIBS on)
find_package(
        Boost ${REQUIRED_BOOST}
        REQUIRED COMPONENTS
        chrono
        coroutine
        program_options
        random
        regex
        system
        thread
        unit_test_framework
)

if(Boost_FOUND)
    message(STATUS "Your boost is here: ${Boost_INCLUDE_DIRS}")
    #check_asio_version()
    #check_beast_version()
    include_directories(${Boost_INCLUDE_DIRS})
endif()

###############################################################################
# Primary project for the_db
project(the_db)
include_directories(
        .
        command_line_options
        ethereum
        node
        peers
        raft
        storage
        websockets
)

set(SOURCE_FILES
        command_line_options/CommandLineOptions.cpp
        command_line_options/CommandLineOptions.h
        ethereum/EthereumApi.cpp
        ethereum/EthereumApi.h
        ethereum/EthereumToken.h
        main.cpp
        MessageInfo.hpp
        node/DaemonInfo.h
        node/Node.cpp
        node/Node.h
        node/NodeInfo.hpp
        node/Singleton.h
        peers/Peer.cpp
        peers/Peer.h
        peers/PeerList.cpp
        peers/PeerList.h
        peers/PeerListener.cpp
        peers/PeerListener.h
        peers/PeerServer.cpp
        peers/PeerServer.h
        peers/PeerSession.cpp
        peers/PeerSession.h
        raft/ApiCommandQueue.h
        raft/ApiCreateCommand.cpp
        raft/ApiCreateCommand.cpp
        raft/ApiCreateCommand.h
        raft/ApiReadCommand.cpp
        raft/ApiReadCommand.h
        raft/Command.hpp
        raft/CommandFactory.cpp
        raft/CommandFactory.h
        raft/CrudCreateCommand.cpp
        raft/CrudCreateCommand.h
        raft/CrudReadCommand.cpp
        raft/CrudReadCommand.h
        raft/JsonTools.cpp
        raft/JsonTools.h
        raft/Raft.cpp
        raft/Raft.h
        raft/RaftHeartbeatCommand.cpp
        raft/RaftHeartbeatCommand.h
        storage/Storage.cpp
        storage/Storage.h
        websockets/Listener.h
        websockets/Session.cpp
        websockets/Session.h
        websockets/WebSocketServer.h
        )

add_executable(the_db ${SOURCE_FILES})
target_link_libraries(the_db ${Boost_LIBRARIES})
if(UNIX AND NOT APPLE)
    target_link_libraries(the_db pthread)
endif()

###############################################################################
# Unit tests
message(STATUS "*** CMakeLists for bluzelled_test")
enable_testing()

include_directories(
        storage
        websockets
        command_line_options
        ethereum
)
set(SOURCE_FILES
        command_line_options/command_line_options_test.cpp
        command_line_options/CommandLineOptions.cpp
        command_line_options/CommandLineOptions.h
        ethereum/ethereum_test.cpp ethereum/TokenBalance.hpp
        ethereum/EthereumApi.cpp
        ethereum/EthereumApi.h
        ethereum/EthereumToken.h
        node/NodeInfo.hpp
        node/NodeInfo_tests.cpp
        node/Singleton.h
        storage/Storage.cpp
        storage/Storage.h
        storage/storage_test.cpp
        test.cpp
        websockets/Listener.h
        websockets/Session.cpp
        websockets/Session.h
        websockets/WebSocketServer.h
        websockets/WebSocketServer_tests.cpp
        )
add_executable(the_db_test ${SOURCE_FILES})
target_link_libraries(the_db_test ${Boost_LIBRARIES})
if(UNIX AND NOT APPLE)
    target_link_libraries(the_db_test pthread)
endif()

cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(REQUIRED_BOOST "1.66.0")

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
    include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
endif()

###############################################################################
# Primary project for the_db
project(the_db)
include_directories(
        .
        command_line_options
        commands
        ethereum
        node
        peers
        raft
        raft/commands
        storage
        websockets
)

set(SOURCE_FILES
        command_line_options/CommandLineOptions.cpp
        command_line_options/CommandLineOptions.h
        command_line_options/ParseUtils.cpp
        command_line_options/ParseUtils.h
        ethereum/EthereumApi.cpp
        main.cpp
        MessageInfo.hpp
        node/Daemon.cpp
        node/Daemon.h
        node/DaemonInfo.h
        node/Node.cpp
        peers/Peer.cpp
        peers/PeerList.cpp
        peers/PeerListener.cpp
        peers/PeerServer.cpp
        peers/PeerSession.cpp
        raft/commands/ApiCreateCommand.cpp
        raft/commands/ApiReadCommand.cpp
        raft/commands/CommandFactory.cpp
        raft/commands/CrudCreateCommand.cpp
        raft/commands/CrudCreateCommand.h
        raft/commands/CrudReadCommand.cpp
        raft/commands/CrudReadCommand.h
        raft/commands/RaftCountVotesCommand.cpp
        raft/commands/RaftCountVotesCommand.h
        raft/commands/RaftHeartbeatCommand.cpp
        raft/commands/RaftHeartbeatCommand.h
        raft/commands/RaftVoteCommand.cpp
        raft/commands/RaftVoteCommand.h
        raft/JsonTools.cpp
        raft/JsonTools.h
        raft/Raft.cpp
        raft/Raft.h
        raft/RaftCandidateState.cpp
        raft/RaftCandidateState.h
        raft/RaftFollowerState.cpp
        raft/RaftFollowerState.h
        raft/RaftLeaderState.cpp
        raft/RaftLeaderState.h
        raft/RaftState.cpp
        raft/RaftState.h
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


add_custom_command(
        TARGET the_db POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/daemon/resources/peers
        ${CMAKE_CURRENT_BINARY_DIR}
)

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
        command_line_options/ParseUtils.cpp
        command_line_options/ParseUtils.h
        ethereum/ethereum_test.cpp
        ethereum/TokenBalance.hpp
        command_line_options/ParseUtils.cpp
        command_line_options/ParseUtils.h
        ethereum/ethereum_test.cpp
        ethereum/TokenBalance.hpp
        ethereum/EthereumApi.cpp
        ethereum/EthereumApi.h
        ethereum/EthereumToken.h
        ethereum/TokenBalance.hpp
        node/Daemon.cpp
        node/Daemon.h
        node/Daemon_tests.cpp
        node/DaemonInfo_tests.cpp
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

add_library(raft
        log_entry.hpp
        raft_base.hpp
        raft.cpp
        raft.hpp
        raft_log.cpp
        raft_log.hpp
        )

target_link_libraries(raft proto)
add_dependencies(raft jsoncpp)
target_include_directories(raft PRIVATE ${JSONCPP_INCLUDE_DIRS} ${PROTO_INCLUDE_DIR})
add_subdirectory(test)
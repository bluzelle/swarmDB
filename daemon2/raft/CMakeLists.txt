add_library(raft
        raft_base.h raft.cpp raft.h log.cpp log.h)
target_link_libraries(raft)

add_subdirectory(test)
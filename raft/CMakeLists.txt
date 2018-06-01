add_library(raft
        log_entry.hpp
        raft_base.hpp
        raft.cpp
        raft.hpp
        )

target_link_libraries(raft)
target_include_directories(raft PRIVATE ${JSONCPP_INCLUDE_DIRS})
add_subdirectory(test)
set(test_srcs raft_test.cpp raft_log_test.cpp raft_add_peers_test.cpp)
set(test_libs raft storage bootstrap proto ${Protobuf_LIBRARIES})

add_gmock_test(raft)

set(test_srcs raft_test.cpp raft_log_test.cpp)
set(test_libs raft storage bootstrap proto protobuf)

add_gmock_test(raft_tests)
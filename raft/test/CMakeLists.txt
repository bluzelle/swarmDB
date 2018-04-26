set(test_srcs raft_test.cpp)
set(test_libs raft bootstrap)

add_gmock_test(raft_tests)
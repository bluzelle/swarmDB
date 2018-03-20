set(test_srcs raft_test.cpp log_test.cpp)
set(test_libs raft)

add_gmock_test(raft_tests)
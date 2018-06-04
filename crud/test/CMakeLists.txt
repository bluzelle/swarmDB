set(test_srcs crud_tests.cpp)
set(test_libs crud node storage bootstrap raft proto protobuf)

add_gmock_test(crud_tests)
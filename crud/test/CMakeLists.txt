set(test_srcs crud_tests.cpp subscription_manager_test.cpp)
set(test_libs crud node storage bootstrap raft proto protobuf.a)

add_gmock_test(crud)

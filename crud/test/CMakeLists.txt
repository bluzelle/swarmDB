set(test_srcs raft_crud_test.cpp crud_test.cpp subscription_manager_test.cpp)
set(test_libs crud raft pbft pbft_operations node storage bootstrap proto ${Protobuf_LIBRARIES})

add_gmock_test(crud)

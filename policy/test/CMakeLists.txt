set(test_srcs eviction_test.cpp)

set(test_libs policy proto storage ${Protobuf_LIBRARIES})

add_gmock_test(policy)

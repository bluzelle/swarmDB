set(test_srcs pbft_test.cpp pbft_operation_test.cpp)
set(test_libs pbft protobuf.a bootstrap)

add_gmock_test(pbft)
add_gmock_test(pbft_operation)


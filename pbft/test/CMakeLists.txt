set(test_srcs pbft_test.cpp pbft_operation_test.cpp)
set(test_libs pbft protobuf bootstrap)

add_gmock_test(pbft_test)
add_gmock_test(pbft_operation_test)


set(test_srcs
        pbft_operation_test_common.cpp
        pbft_persistent_operation_test.cpp
        pbft_operation_manager_test.cpp
    )
set(test_libs pbft_operations storage pbft smart_mocks ${Protobuf_LIBRARIES})

add_gmock_test(pbft_operation)

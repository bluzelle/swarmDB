set(test_srcs pbft_test.cpp pbft_operation_test.cpp pbft_failure_detector_test.cpp pbft_audit_test.cpp pbft_test_common.cpp pbft_checkpoint_tests.cpp pbft_configuration_test.cpp pbft_config_store_test.cpp)
set(test_libs pbft ${Protobuf_LIBRARIES} bootstrap)

add_gmock_test(pbft)


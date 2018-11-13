set(test_srcs
    pbft_test.cpp
    pbft_operation_test.cpp
    pbft_failure_detector_test.cpp
    pbft_audit_test.cpp
    pbft_test_common.cpp
    pbft_checkpoint_tests.cpp
    pbft_configuration_test.cpp
    pbft_config_store_test.cpp
    pbft_join_leave_test.cpp
    #pbft_catchup_test.cpp
    pbft_timestamp_test.cpp
    database_pbft_service_test.cpp
    pbft_proto_test.cpp
    pbft_newview_test.cpp)

set(test_libs pbft crypto options ${Protobuf_LIBRARIES} bootstrap storage)

add_gmock_test(pbft)

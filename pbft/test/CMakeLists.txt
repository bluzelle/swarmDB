set(test_srcs
    pbft_test.cpp
    pbft_failure_detector_test.cpp
    pbft_audit_test.cpp
    pbft_test_common.cpp
    pbft_checkpoint_tests.cpp
    pbft_proto_test.cpp
    pbft_catchup_test.cpp
    pbft_timestamp_test.cpp
    database_pbft_service_test.cpp
    pbft_proto_test.cpp
    pbft_newview_test.cpp
    pbft_persistent_state_test.cpp
    pbft_viewchange_test.cpp)
set(test_libs pbft pbft_operations crypto options ${Protobuf_LIBRARIES} storage ${ROCKSDB_LIBRARIES} smart_mocks)

add_gmock_test(pbft)

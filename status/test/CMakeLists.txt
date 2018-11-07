set(test_srcs status_test.cpp)
set(test_libs status ${Protobuf_LIBRARIES})

add_gmock_test(status)

set(test_srcs audit_test.cpp)
set(test_libs audit proto  ${Protobuf_LIBRARIES})

add_gmock_test(audit)

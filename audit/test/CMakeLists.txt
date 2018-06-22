set(test_srcs audit_test.cpp)
set(test_libs audit proto protobuf)

add_gmock_test(audit_test)

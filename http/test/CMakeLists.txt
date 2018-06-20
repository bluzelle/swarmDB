set(test_srcs server_test.cpp connection_test.cpp)
set(test_libs http proto protobuf)

add_gmock_test(http_tests)

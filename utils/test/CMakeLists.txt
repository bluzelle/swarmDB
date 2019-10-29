set(test_srcs utils_test.cpp make_endpoint_test.cpp)
set(test_libs utils)
set(test_deps proto)

add_gmock_test(utils)

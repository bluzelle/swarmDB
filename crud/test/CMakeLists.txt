set(test_srcs crud_tests.cpp)
set(test_libs crud node storage bootstrap raft)

add_gmock_test(crud_tests)
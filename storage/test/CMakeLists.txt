set(test_srcs storage_test.cpp)
set(test_libs storage node)

add_gmock_test(storage)

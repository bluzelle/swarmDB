set(test_srcs monitor_test.cpp)
set(test_libs monitor options)

add_gmock_test(monitor)

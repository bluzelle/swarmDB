set(test_srcs peers_beacon_tests.cpp)
set(test_libs peers_beacon utils options)

add_gmock_test(peers_beacon)

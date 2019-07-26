set(test_srcs peers_beacon_tests.cpp)
set(test_libs peers_beacon utils options smart_mocks)

add_gmock_test(peers_beacon)

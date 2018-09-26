set(test_srcs node_test.cpp session_test.cpp)
set(test_libs node proto ${Protobuf_LIBRARIES})

add_gmock_test(node)

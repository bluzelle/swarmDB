set(test_srcs node_test.cpp session_test.cpp node_test_common.hpp)
set(test_libs node proto options crypto ${Protobuf_LIBRARIES})

add_gmock_test(node)

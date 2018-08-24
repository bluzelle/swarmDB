set(test_srcs crypto_test.cpp)
set(test_libs crypto proto options ${Protobuf_LIBRARIES})

add_gmock_test(crypto)

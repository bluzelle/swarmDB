protobuf_generate_cpp(PROTO_SRC PROTO_HEADER bluzelle.proto database.proto pbft.proto audit.proto status.proto)
add_library(proto ${PROTO_HEADER} ${PROTO_SRC})
set_target_properties(proto PROPERTIES COMPILE_FLAGS "-Wno-unused")
set(PROTO_INCLUDE_DIR ${CMAKE_BINARY_DIR}/proto)

add_library(crypto
        crypto_base.hpp
        crypto.hpp
        crypto.cpp
        )

target_link_libraries(crypto proto)
add_dependencies(crypto openssl)
target_include_directories(crypto PRIVATE ${PROTO_INCLUDE_DIR})
add_subdirectory(test)

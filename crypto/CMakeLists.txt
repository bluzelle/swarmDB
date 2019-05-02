add_library(crypto
        crypto_base.hpp
        crypto.hpp
        crypto.cpp
        )

target_link_libraries(crypto proto utils)
add_dependencies(crypto boost openssl)
target_include_directories(crypto PRIVATE ${BLUZELLE_STD_INCLUDES})
add_subdirectory(test)

add_library(bootstrap STATIC
    peer_address.hpp
    bootstrap_peers_base.hpp
    bootstrap_peers.cpp
    bootstrap_peers.hpp
  )

target_link_libraries(bootstrap utils)
target_include_directories(bootstrap PRIVATE ${JSONCPP_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR})

add_subdirectory(test)

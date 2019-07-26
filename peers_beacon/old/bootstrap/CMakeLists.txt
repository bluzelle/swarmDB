add_library(bootstrap STATIC
    peer_address.hpp
    bootstrap_peers_base.hpp
    bootstrap_peers.cpp
    bootstrap_peers.hpp
  )

target_link_libraries(bootstrap utils)
target_include_directories(bootstrap PRIVATE ${BLUZELLE_STD_INCLUDES})
add_dependencies(bootstrap boost openssl)

add_subdirectory(test)

add_library(peers_beacon STATIC
        peer_address.hpp
        peers_beacon_base.hpp
        peers_beacon.cpp
        peers_beacon.hpp
        )

target_link_libraries(peers_beacon utils)
target_include_directories(peers_beacon PRIVATE ${BLUZELLE_STD_INCLUDES})
add_dependencies(peers_beacon boost openssl)

add_subdirectory(test)


add_library(peers_beacon STATIC
        peers_beacon.cpp
        peers_beacon.hpp
        peers_beacon_base.hpp)

target_link_libraries(peers_beacon utils)
add_dependencies(peers_beacon boost jsoncpp)
target_include_directories(peers_beacon PRIVATE ${BLUZELLE_STD_INCLUDES})
add_subdirectory(test)

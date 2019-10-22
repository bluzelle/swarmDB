add_library(smart_mocks STATIC
        smart_mock_node.hpp
        smart_mock_node.cpp
        smart_mock_io.hpp
        smart_mock_io.cpp
        smart_mock_peers_beacon.hpp
        smart_mock_peers_beacon.cpp
        )

add_dependencies(smart_mocks boost proto googletest)

target_include_directories(smart_mocks PRIVATE ${BLUZELLE_STD_INCLUDES})

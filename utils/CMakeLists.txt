add_library(utils STATIC
    http_req.cpp
    http_req.hpp
    blacklist.cpp
    blacklist.hpp
    make_endpoint.hpp
    make_endpoint.cpp
    bytes_to_debug_string.cpp
    bytes_to_debug_string.hpp
    crypto.cpp
    crypto.hpp
    esr_peer_info.cpp
    esr_peer_info.hpp)

target_link_libraries(utils ${JSONCPP_LIBRARIES} ${OPENSSL_LIBRARIES})

add_dependencies(utils boost jsoncpp openssl)
target_include_directories(utils PRIVATE ${BLUZELLE_STD_INCLUDES})

add_subdirectory(test)

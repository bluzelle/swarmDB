add_library(utils STATIC
    blacklist.cpp
    blacklist.hpp
    make_endpoint.hpp
    make_endpoint.cpp
    bytes_to_debug_string.cpp
    bytes_to_debug_string.hpp
    crypto.cpp
    crypto.hpp
    utils_interface_base.hpp
    utils_interface.hpp
    esr_peer_info.cpp
    http_req.cpp
    test/make_endpoint_test.cpp)

target_link_libraries(utils ${JSONCPP_LIBRARIES} ${OPENSSL_LIBRARIES})

add_dependencies(utils boost jsoncpp openssl)
target_include_directories(utils PRIVATE ${BLUZELLE_STD_INCLUDES})

add_subdirectory(test)

add_library(node STATIC
        ../include/bluzelle.hpp
        ../include/boost_asio_beast.hpp
        ../mocks/mock_boost_asio_beast.hpp
        node_base.hpp
        node.hpp
        node.cpp
        session_base.hpp
        session.hpp
        session.cpp
        ../mocks/mock_session_base.hpp)

target_link_libraries(node proto pbft)
add_dependencies(node boost proto googletest jsoncpp) # for FRIEND_TEST

target_include_directories(node PRIVATE ${JSONCPP_INCLUDE_DIRS} ${PROTO_INCLUDE_DIR})

add_subdirectory(test)

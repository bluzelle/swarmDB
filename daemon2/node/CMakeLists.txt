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

target_link_libraries(node)
add_dependencies(node googletest) # for FRIEND_TEST
target_include_directories(node PRIVATE ${JSONCPP_INCLUDE_DIRS})

add_subdirectory(test)

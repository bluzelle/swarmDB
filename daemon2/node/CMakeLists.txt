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
        )

target_link_libraries(node)
target_include_directories(node PRIVATE ${JSONCPP_INCLUDE_DIRS})

add_subdirectory(test)

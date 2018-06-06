add_library(http STATIC
        ../include/bluzelle.hpp
        ../include/boost_asio_beast.hpp
        server.cpp
        server.hpp
        connection.cpp
        connection.hpp
        )

target_link_libraries(http )
target_include_directories(http PRIVATE ${JSONCPP_INCLUDE_DIRS})

add_subdirectory(test)

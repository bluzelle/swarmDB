add_library(status STATIC
        status.cpp
        status.hpp
        )

target_link_libraries(status )
add_dependencies(status jsoncpp)
target_include_directories(status PRIVATE ${JSONCPP_INCLUDE_DIRS})

add_subdirectory(test)

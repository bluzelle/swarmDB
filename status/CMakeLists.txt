add_library(status STATIC
        status.cpp
        status.hpp
        )

target_link_libraries(status proto)
add_dependencies(status jsoncpp proto)
target_include_directories(status PRIVATE ${JSONCPP_INCLUDE_DIRS})

add_subdirectory(test)

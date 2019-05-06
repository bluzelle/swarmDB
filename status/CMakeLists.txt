add_library(status STATIC
        status.cpp
        status.hpp
        )

target_link_libraries(status proto)
add_dependencies(status boost jsoncpp proto)
target_include_directories(status PRIVATE ${BLUZELLE_STD_INCLUDES})

add_subdirectory(test)

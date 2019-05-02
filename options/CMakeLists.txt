add_library(options STATIC
        ../include/bluzelle.hpp
        options.cpp
        options.hpp
        options_base.hpp
        simple_options.cpp
        simple_options.hpp
        )

target_link_libraries(options utils)
target_include_directories(options PRIVATE ${BLUZELLE_STD_INCLUDES})
add_dependencies(options boost jsoncpp)

add_subdirectory(test)
